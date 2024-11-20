#include "command-line.h"
#include "sexp/sexp-base.h"
#include "vector.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <readline/readline.h>

struct vector* cmd_get_documentation(const char* cmd_name,
                                     const char* docfile_name,
                                     bool brief) {
    FILE* docfile = fopen(docfile_name, "r");
    
    // the documentation file will be an ORG file, with each command having its
    // own top level section.

    int command_name_len = strlen(cmd_name);

    char* linebuffer = malloc(50 * sizeof(char));
    size_t linebuffer_size = 50;
    if (linebuffer == NULL) {
        printf("ERROR: couldn't allocate linebuffer!\n");
        return NULL;
    }

    // seek until we find the documentation location
    while (true) {
        // read the first part of the line into memory.  We are looking for the
        // following string:
        // "* COMMAND_NAME\n"
        int bytes_read = getline(&linebuffer, &linebuffer_size, docfile);

        if (bytes_read < 0) {
            struct vector* error_msg = make_vector(sizeof(char), 50);
            
            snprintf(vec_dat(error_msg), 50,
                     "No documentation found for \"%s\"", cmd_name);

            return error_msg;
        }

        if (bytes_read != 3 + command_name_len)
            continue;

        if (memcmp(linebuffer, "* ", 2) == 0  &&
            memcmp(linebuffer+2, cmd_name, command_name_len) == 0)
            break;
    }

    // the first paragraph will be the brief summary
    struct vector* docs = make_vector(sizeof(char), 20);

    // The will read the first paragraph, which is a brief summary of the
    // command.
    while (true) {
        int bytes_read = getline(&linebuffer, &linebuffer_size, docfile);
        
        // the first paragraph is finished when either an empty line is read, or
        // the next command documentation begins. (or if the file end is
        // reached)
        if ((bytes_read == 1 && vec_len(docs) > 0) ||
            bytes_read == -1) {
            break;
        }

        // in the case that there is no extended description, we will need
        // this line present for doing checks later.
        if (bytes_read >= 2 && memcmp(linebuffer, "* ", 2) == 0) {
            fseek(docfile, -1*bytes_read, SEEK_CUR);
            break;
        }

        
        // Remove leading spaces
        size_t padding_offset = 0;
        for (; linebuffer[padding_offset] == ' '; padding_offset++);

        // remove leading whitespace, and exchange newline for a space.
        vec_pushn(docs, linebuffer+padding_offset, bytes_read-padding_offset - 1);
        vec_push(docs, " ");
    }

    // ensure a null terminator is included at the end.
    vec_push(docs, "\0");
            

    // if we are only getting the brief documentation, then we can return here.
    if (brief == true) {
        free(linebuffer);
        return docs;
    }

    // otherwise, return the full documentation.
    vec_set(docs, 0, "\0");
    vec_resize(docs, 0);
    while (true) {
        int bytes_read = getline(&linebuffer, &linebuffer_size, docfile);
        
        if ((bytes_read >= 2 && memcmp(linebuffer, "* ", 2) == 0) ||
            bytes_read == -1)
            break;

        // Remove leading spaces
        size_t padding_offset = 0;
        for (; linebuffer[padding_offset] == ' '; padding_offset++);

        // remove padding 
        vec_pushn(docs, linebuffer+padding_offset, bytes_read-padding_offset);
    }

    // ensure the string is null terminated
    vec_push(docs, "\0"); 

    free(linebuffer);
    return docs;
}

void help_command(const struct command_line_args *env, int argc,
                  char **argv) {
  // print comand briefs
  if (argc == 1) {
    printf("here is a summary of the commands you can use\n");

    // find the maximum name length (for spacing purposes)
    unsigned int max_length = 0;
    for (size_t i = 0; i < env->num_commands; i++) {
      if (strlen(env->command_list[i].name) > max_length)
        max_length = strlen(env->command_list[i].name);
    }

    for (size_t i = 0; i < env->num_commands; i++) {
      // add appropriate amount of spacing to right align docs
      size_t pad_chars = max_length - strlen(env->command_list[i].name);
      for (size_t n = 0; n < pad_chars; n++)
        printf(" ");

      printf(" \033[1m%s\033[0m", env->command_list[i].name);

      struct vector *brief = cmd_get_documentation(env->command_list[i].name,
                                                   env->documentation_file,
                                                   true);

      if (brief != NULL)
        printf(" :: %s\n", (char *)vec_dat(brief));

      free_vector(brief);
    }
    return;
  }

  // print in depth summaries
  if (argc == 2) {
    struct vector *docs =
        cmd_get_documentation(argv[1], env->documentation_file, false);

    if (docs != NULL)
      puts(vec_dat(docs));

    free_vector(docs);
    return;
  }

  printf("usage: help\nusage: help COMMAND\n");
}

void try_call_command(const struct command_line_args* env,
                      int argc, char **argv) {
    const struct command* cmd_list = env->command_list;
    size_t num_cmds = env->num_commands;
    
    if (argc <= 0)
        return;

    // find the matching command in the list
    size_t i = 0;
    for (; (strcmp(cmd_list[i].name, argv[0]) != 0) && i < num_cmds; i++);

    struct error e = {0};
    // run the associated callback (implicit help command)
    if (i < num_cmds) {
        cmd_list[i].callback(argc, argv, &e);                
    } else if (strcmp(argv[0], "help") == 0) {
        help_command(env, argc, argv);
    } else {
        printf("ERROR: Unknown Command \"%s\"\n", argv[0]);
    }

    // if either of these is no longer zero, then the command must have
    // encountered an error.
    if (e.operations != 0 || e.self != 0) {
        char *err_msg = describe_error(e);
        printf("\033[31m--COMMAND ERROR--\n%s\033[0m\n", err_msg);
        free(err_msg);
        free_error(e);
    }
}

void* command_line_thread(void* a) {
    struct command_line_args* env = a;

    while (*(env->run_program)) {
        // get the next command with GNU readline
        char* linebuf_tmp = readline("> ");
        size_t bytes_read = strlen(linebuf_tmp);
        
        // copy the data into a stack buffer, and free the dynamic buffer
        char linebuf[bytes_read];
        memcpy(linebuf, linebuf_tmp, bytes_read+1); // include null terminator
        free(linebuf_tmp);

        // filter out blank entries.
        size_t space_offset = 0;
        for (; isspace(linebuf[space_offset]) && space_offset < bytes_read;
               space_offset++);

        if (space_offset+1 == bytes_read)
            continue;
        
        char* argv[10];
        int i = 0;
        for (char* token = strtok(linebuf, " \n");
             token != NULL;
             token = strtok(NULL, " \n")) {
            argv[i++] = token;
        }

        try_call_command(env, i, argv);
    }

    return NULL;
}

