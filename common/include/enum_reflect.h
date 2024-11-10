#ifndef ENUM_REFLECT_H
#define ENUM_REFLECT_H

/******************************** N_ARG MACROS ********************************/
/* The following elisp code will generate an N-ARG macro that is capable of
  counting up to N arguments (N is specified by programmer).
  
(defun es/generate_nth_arg (n)
  (let ((macro-start (point)))
    (insert "#define _GET_NTH_ARG(")
    (dotimes (i (- n 1))
      (insert (format "_%d, " (+ i 1))))
    (insert "N, ...) N\n")
    (indent-region macro-start (point))))

(defun es/generate_rseq (n)
  (let ((macro-start (point)))
    (insert "#define _RSEQ_N() ")
    (dotimes (i (- n 1))
      (insert (format "%d, " (- n i 1))))
    (delete-char -2) ;; remove trailing space/comma
    (insert "\n")
    (indent-region macro-start (point))))

(defun es/generate_narg (n)
  (interactive "nN: ")
  (es/generate_nth_arg n)
  (es/generate_rseq n)
  (insert "#define _N_ARG(...) _GET_NTH_ARG(__VA_ARGS__)\n")
  (insert "#define N_ARG(...) _N_ARG(__VA_ARGS__, _RSEQ_N())\n"))
*/

/* N_ARG returns the number of arguments that it is passed */
#define _GET_NTH_ARG(                                                          \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16,     \
    _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, \
    _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, \
    _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, \
    _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, \
    _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104,      \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117, _118, _119, _120, _121, _122, _123, _124, _125, _126, _127, _128,    \
    _129, _130, _131, _132, _133, _134, _135, _136, _137, _138, _139, _140,    \
    _141, _142, _143, _144, _145, _146, _147, _148, _149, _150, _151, _152,    \
    _153, _154, _155, _156, _157, _158, _159, _160, _161, _162, _163, _164,    \
    _165, _166, _167, _168, _169, _170, _171, _172, _173, _174, _175, _176,    \
    _177, _178, _179, _180, _181, _182, _183, _184, _185, _186, _187, _188,    \
    _189, _190, _191, _192, _193, _194, _195, _196, _197, _198, _199, _200,    \
    _201, _202, _203, _204, _205, _206, _207, _208, _209, _210, _211, _212,    \
    _213, _214, _215, _216, _217, _218, _219, _220, _221, _222, _223, _224,    \
    _225, _226, _227, _228, _229, _230, _231, _232, _233, _234, _235, _236,    \
    _237, _238, _239, _240, _241, _242, _243, _244, _245, _246, _247, _248,    \
    _249, N, ...)                                                              \
  N
#define _RSEQ_N()                                                              \
  249, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 237, 236, 235,   \
      234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224, 223, 222, 221,    \
      220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208, 207,    \
      206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 194, 193,    \
      192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179,    \
      178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165,    \
      164, 163, 162, 161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151,    \
      150, 149, 148, 147, 146, 145, 144, 143, 142, 141, 140, 139, 138, 137,    \
      136, 135, 134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123,    \
      122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109,    \
      108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, \
      92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75,  \
      74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57,  \
      56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39,  \
      38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21,  \
      20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
#define _N_ARG(...) _GET_NTH_ARG(__VA_ARGS__)
#define N_ARG(...) _N_ARG(__VA_ARGS__, _RSEQ_N())

/******************************* FOREACH MACROS *******************************/
#define JOIN(a, b) _JOIN(a, b)
#define _JOIN(a, b) a##b

/* calls f on all parameters */
#define FOREACH(f, ...) _FOREACH_N(__VA_ARGS__)(f, __VA_ARGS__)
#define _FOREACH_N(...) JOIN(_FOREACH_, N_ARG(__VA_ARGS__))

/* The following elisp code will generate N levels for the foreach macro. Ensure
   that the generated N_ARG macro can handle greater or equal arguments that
   foreach can handle.

(defun es/generate_foreach (n)
  (interactive "nN: ")
  (insert "#define _FOREACH_1(_fn, x) _fn(x)\n")
  
  (cl-loop for i from 2 to n
           do
           (insert (format "#define _FOREACH_%d(_fn, x, ...) _fn(x) _FOREACH_%d(_fn, __VA_ARGS__)\n"
                           i (- i 1)))))
*/

#define _FOREACH_1(_fn, x) _fn(x)
#define _FOREACH_2(_fn, x, ...) _fn(x) _FOREACH_1(_fn, __VA_ARGS__)
#define _FOREACH_3(_fn, x, ...) _fn(x) _FOREACH_2(_fn, __VA_ARGS__)
#define _FOREACH_4(_fn, x, ...) _fn(x) _FOREACH_3(_fn, __VA_ARGS__)
#define _FOREACH_5(_fn, x, ...) _fn(x) _FOREACH_4(_fn, __VA_ARGS__)
#define _FOREACH_6(_fn, x, ...) _fn(x) _FOREACH_5(_fn, __VA_ARGS__)
#define _FOREACH_7(_fn, x, ...) _fn(x) _FOREACH_6(_fn, __VA_ARGS__)
#define _FOREACH_8(_fn, x, ...) _fn(x) _FOREACH_7(_fn, __VA_ARGS__)
#define _FOREACH_9(_fn, x, ...) _fn(x) _FOREACH_8(_fn, __VA_ARGS__)
#define _FOREACH_10(_fn, x, ...) _fn(x) _FOREACH_9(_fn, __VA_ARGS__)
#define _FOREACH_11(_fn, x, ...) _fn(x) _FOREACH_10(_fn, __VA_ARGS__)
#define _FOREACH_12(_fn, x, ...) _fn(x) _FOREACH_11(_fn, __VA_ARGS__)
#define _FOREACH_13(_fn, x, ...) _fn(x) _FOREACH_12(_fn, __VA_ARGS__)
#define _FOREACH_14(_fn, x, ...) _fn(x) _FOREACH_13(_fn, __VA_ARGS__)
#define _FOREACH_15(_fn, x, ...) _fn(x) _FOREACH_14(_fn, __VA_ARGS__)
#define _FOREACH_16(_fn, x, ...) _fn(x) _FOREACH_15(_fn, __VA_ARGS__)
#define _FOREACH_17(_fn, x, ...) _fn(x) _FOREACH_16(_fn, __VA_ARGS__)
#define _FOREACH_18(_fn, x, ...) _fn(x) _FOREACH_17(_fn, __VA_ARGS__)
#define _FOREACH_19(_fn, x, ...) _fn(x) _FOREACH_18(_fn, __VA_ARGS__)
#define _FOREACH_20(_fn, x, ...) _fn(x) _FOREACH_19(_fn, __VA_ARGS__)
#define _FOREACH_21(_fn, x, ...) _fn(x) _FOREACH_20(_fn, __VA_ARGS__)
#define _FOREACH_22(_fn, x, ...) _fn(x) _FOREACH_21(_fn, __VA_ARGS__)
#define _FOREACH_23(_fn, x, ...) _fn(x) _FOREACH_22(_fn, __VA_ARGS__)
#define _FOREACH_24(_fn, x, ...) _fn(x) _FOREACH_23(_fn, __VA_ARGS__)
#define _FOREACH_25(_fn, x, ...) _fn(x) _FOREACH_24(_fn, __VA_ARGS__)
#define _FOREACH_26(_fn, x, ...) _fn(x) _FOREACH_25(_fn, __VA_ARGS__)
#define _FOREACH_27(_fn, x, ...) _fn(x) _FOREACH_26(_fn, __VA_ARGS__)
#define _FOREACH_28(_fn, x, ...) _fn(x) _FOREACH_27(_fn, __VA_ARGS__)
#define _FOREACH_29(_fn, x, ...) _fn(x) _FOREACH_28(_fn, __VA_ARGS__)
#define _FOREACH_30(_fn, x, ...) _fn(x) _FOREACH_29(_fn, __VA_ARGS__)
#define _FOREACH_31(_fn, x, ...) _fn(x) _FOREACH_30(_fn, __VA_ARGS__)
#define _FOREACH_32(_fn, x, ...) _fn(x) _FOREACH_31(_fn, __VA_ARGS__)
#define _FOREACH_33(_fn, x, ...) _fn(x) _FOREACH_32(_fn, __VA_ARGS__)
#define _FOREACH_34(_fn, x, ...) _fn(x) _FOREACH_33(_fn, __VA_ARGS__)
#define _FOREACH_35(_fn, x, ...) _fn(x) _FOREACH_34(_fn, __VA_ARGS__)
#define _FOREACH_36(_fn, x, ...) _fn(x) _FOREACH_35(_fn, __VA_ARGS__)
#define _FOREACH_37(_fn, x, ...) _fn(x) _FOREACH_36(_fn, __VA_ARGS__)
#define _FOREACH_38(_fn, x, ...) _fn(x) _FOREACH_37(_fn, __VA_ARGS__)
#define _FOREACH_39(_fn, x, ...) _fn(x) _FOREACH_38(_fn, __VA_ARGS__)
#define _FOREACH_40(_fn, x, ...) _fn(x) _FOREACH_39(_fn, __VA_ARGS__)
#define _FOREACH_41(_fn, x, ...) _fn(x) _FOREACH_40(_fn, __VA_ARGS__)
#define _FOREACH_42(_fn, x, ...) _fn(x) _FOREACH_41(_fn, __VA_ARGS__)
#define _FOREACH_43(_fn, x, ...) _fn(x) _FOREACH_42(_fn, __VA_ARGS__)
#define _FOREACH_44(_fn, x, ...) _fn(x) _FOREACH_43(_fn, __VA_ARGS__)
#define _FOREACH_45(_fn, x, ...) _fn(x) _FOREACH_44(_fn, __VA_ARGS__)
#define _FOREACH_46(_fn, x, ...) _fn(x) _FOREACH_45(_fn, __VA_ARGS__)
#define _FOREACH_47(_fn, x, ...) _fn(x) _FOREACH_46(_fn, __VA_ARGS__)
#define _FOREACH_48(_fn, x, ...) _fn(x) _FOREACH_47(_fn, __VA_ARGS__)
#define _FOREACH_49(_fn, x, ...) _fn(x) _FOREACH_48(_fn, __VA_ARGS__)
#define _FOREACH_50(_fn, x, ...) _fn(x) _FOREACH_49(_fn, __VA_ARGS__)
#define _FOREACH_51(_fn, x, ...) _fn(x) _FOREACH_50(_fn, __VA_ARGS__)
#define _FOREACH_52(_fn, x, ...) _fn(x) _FOREACH_51(_fn, __VA_ARGS__)
#define _FOREACH_53(_fn, x, ...) _fn(x) _FOREACH_52(_fn, __VA_ARGS__)
#define _FOREACH_54(_fn, x, ...) _fn(x) _FOREACH_53(_fn, __VA_ARGS__)
#define _FOREACH_55(_fn, x, ...) _fn(x) _FOREACH_54(_fn, __VA_ARGS__)
#define _FOREACH_56(_fn, x, ...) _fn(x) _FOREACH_55(_fn, __VA_ARGS__)
#define _FOREACH_57(_fn, x, ...) _fn(x) _FOREACH_56(_fn, __VA_ARGS__)
#define _FOREACH_58(_fn, x, ...) _fn(x) _FOREACH_57(_fn, __VA_ARGS__)
#define _FOREACH_59(_fn, x, ...) _fn(x) _FOREACH_58(_fn, __VA_ARGS__)
#define _FOREACH_60(_fn, x, ...) _fn(x) _FOREACH_59(_fn, __VA_ARGS__)
#define _FOREACH_61(_fn, x, ...) _fn(x) _FOREACH_60(_fn, __VA_ARGS__)
#define _FOREACH_62(_fn, x, ...) _fn(x) _FOREACH_61(_fn, __VA_ARGS__)
#define _FOREACH_63(_fn, x, ...) _fn(x) _FOREACH_62(_fn, __VA_ARGS__)
#define _FOREACH_64(_fn, x, ...) _fn(x) _FOREACH_63(_fn, __VA_ARGS__)
#define _FOREACH_65(_fn, x, ...) _fn(x) _FOREACH_64(_fn, __VA_ARGS__)
#define _FOREACH_66(_fn, x, ...) _fn(x) _FOREACH_65(_fn, __VA_ARGS__)
#define _FOREACH_67(_fn, x, ...) _fn(x) _FOREACH_66(_fn, __VA_ARGS__)
#define _FOREACH_68(_fn, x, ...) _fn(x) _FOREACH_67(_fn, __VA_ARGS__)
#define _FOREACH_69(_fn, x, ...) _fn(x) _FOREACH_68(_fn, __VA_ARGS__)
#define _FOREACH_70(_fn, x, ...) _fn(x) _FOREACH_69(_fn, __VA_ARGS__)
#define _FOREACH_71(_fn, x, ...) _fn(x) _FOREACH_70(_fn, __VA_ARGS__)
#define _FOREACH_72(_fn, x, ...) _fn(x) _FOREACH_71(_fn, __VA_ARGS__)
#define _FOREACH_73(_fn, x, ...) _fn(x) _FOREACH_72(_fn, __VA_ARGS__)
#define _FOREACH_74(_fn, x, ...) _fn(x) _FOREACH_73(_fn, __VA_ARGS__)
#define _FOREACH_75(_fn, x, ...) _fn(x) _FOREACH_74(_fn, __VA_ARGS__)
#define _FOREACH_76(_fn, x, ...) _fn(x) _FOREACH_75(_fn, __VA_ARGS__)
#define _FOREACH_77(_fn, x, ...) _fn(x) _FOREACH_76(_fn, __VA_ARGS__)
#define _FOREACH_78(_fn, x, ...) _fn(x) _FOREACH_77(_fn, __VA_ARGS__)
#define _FOREACH_79(_fn, x, ...) _fn(x) _FOREACH_78(_fn, __VA_ARGS__)
#define _FOREACH_80(_fn, x, ...) _fn(x) _FOREACH_79(_fn, __VA_ARGS__)
#define _FOREACH_81(_fn, x, ...) _fn(x) _FOREACH_80(_fn, __VA_ARGS__)
#define _FOREACH_82(_fn, x, ...) _fn(x) _FOREACH_81(_fn, __VA_ARGS__)
#define _FOREACH_83(_fn, x, ...) _fn(x) _FOREACH_82(_fn, __VA_ARGS__)
#define _FOREACH_84(_fn, x, ...) _fn(x) _FOREACH_83(_fn, __VA_ARGS__)
#define _FOREACH_85(_fn, x, ...) _fn(x) _FOREACH_84(_fn, __VA_ARGS__)
#define _FOREACH_86(_fn, x, ...) _fn(x) _FOREACH_85(_fn, __VA_ARGS__)
#define _FOREACH_87(_fn, x, ...) _fn(x) _FOREACH_86(_fn, __VA_ARGS__)
#define _FOREACH_88(_fn, x, ...) _fn(x) _FOREACH_87(_fn, __VA_ARGS__)
#define _FOREACH_89(_fn, x, ...) _fn(x) _FOREACH_88(_fn, __VA_ARGS__)
#define _FOREACH_90(_fn, x, ...) _fn(x) _FOREACH_89(_fn, __VA_ARGS__)
#define _FOREACH_91(_fn, x, ...) _fn(x) _FOREACH_90(_fn, __VA_ARGS__)
#define _FOREACH_92(_fn, x, ...) _fn(x) _FOREACH_91(_fn, __VA_ARGS__)
#define _FOREACH_93(_fn, x, ...) _fn(x) _FOREACH_92(_fn, __VA_ARGS__)
#define _FOREACH_94(_fn, x, ...) _fn(x) _FOREACH_93(_fn, __VA_ARGS__)
#define _FOREACH_95(_fn, x, ...) _fn(x) _FOREACH_94(_fn, __VA_ARGS__)
#define _FOREACH_96(_fn, x, ...) _fn(x) _FOREACH_95(_fn, __VA_ARGS__)
#define _FOREACH_97(_fn, x, ...) _fn(x) _FOREACH_96(_fn, __VA_ARGS__)
#define _FOREACH_98(_fn, x, ...) _fn(x) _FOREACH_97(_fn, __VA_ARGS__)
#define _FOREACH_99(_fn, x, ...) _fn(x) _FOREACH_98(_fn, __VA_ARGS__)
#define _FOREACH_100(_fn, x, ...) _fn(x) _FOREACH_99(_fn, __VA_ARGS__)
#define _FOREACH_101(_fn, x, ...) _fn(x) _FOREACH_100(_fn, __VA_ARGS__)
#define _FOREACH_102(_fn, x, ...) _fn(x) _FOREACH_101(_fn, __VA_ARGS__)
#define _FOREACH_103(_fn, x, ...) _fn(x) _FOREACH_102(_fn, __VA_ARGS__)
#define _FOREACH_104(_fn, x, ...) _fn(x) _FOREACH_103(_fn, __VA_ARGS__)
#define _FOREACH_105(_fn, x, ...) _fn(x) _FOREACH_104(_fn, __VA_ARGS__)
#define _FOREACH_106(_fn, x, ...) _fn(x) _FOREACH_105(_fn, __VA_ARGS__)
#define _FOREACH_107(_fn, x, ...) _fn(x) _FOREACH_106(_fn, __VA_ARGS__)
#define _FOREACH_108(_fn, x, ...) _fn(x) _FOREACH_107(_fn, __VA_ARGS__)
#define _FOREACH_109(_fn, x, ...) _fn(x) _FOREACH_108(_fn, __VA_ARGS__)
#define _FOREACH_110(_fn, x, ...) _fn(x) _FOREACH_109(_fn, __VA_ARGS__)
#define _FOREACH_111(_fn, x, ...) _fn(x) _FOREACH_110(_fn, __VA_ARGS__)
#define _FOREACH_112(_fn, x, ...) _fn(x) _FOREACH_111(_fn, __VA_ARGS__)
#define _FOREACH_113(_fn, x, ...) _fn(x) _FOREACH_112(_fn, __VA_ARGS__)
#define _FOREACH_114(_fn, x, ...) _fn(x) _FOREACH_113(_fn, __VA_ARGS__)
#define _FOREACH_115(_fn, x, ...) _fn(x) _FOREACH_114(_fn, __VA_ARGS__)
#define _FOREACH_116(_fn, x, ...) _fn(x) _FOREACH_115(_fn, __VA_ARGS__)
#define _FOREACH_117(_fn, x, ...) _fn(x) _FOREACH_116(_fn, __VA_ARGS__)
#define _FOREACH_118(_fn, x, ...) _fn(x) _FOREACH_117(_fn, __VA_ARGS__)
#define _FOREACH_119(_fn, x, ...) _fn(x) _FOREACH_118(_fn, __VA_ARGS__)
#define _FOREACH_120(_fn, x, ...) _fn(x) _FOREACH_119(_fn, __VA_ARGS__)
#define _FOREACH_121(_fn, x, ...) _fn(x) _FOREACH_120(_fn, __VA_ARGS__)
#define _FOREACH_122(_fn, x, ...) _fn(x) _FOREACH_121(_fn, __VA_ARGS__)
#define _FOREACH_123(_fn, x, ...) _fn(x) _FOREACH_122(_fn, __VA_ARGS__)
#define _FOREACH_124(_fn, x, ...) _fn(x) _FOREACH_123(_fn, __VA_ARGS__)
#define _FOREACH_125(_fn, x, ...) _fn(x) _FOREACH_124(_fn, __VA_ARGS__)
#define _FOREACH_126(_fn, x, ...) _fn(x) _FOREACH_125(_fn, __VA_ARGS__)
#define _FOREACH_127(_fn, x, ...) _fn(x) _FOREACH_126(_fn, __VA_ARGS__)
#define _FOREACH_128(_fn, x, ...) _fn(x) _FOREACH_127(_fn, __VA_ARGS__)
#define _FOREACH_129(_fn, x, ...) _fn(x) _FOREACH_128(_fn, __VA_ARGS__)
#define _FOREACH_130(_fn, x, ...) _fn(x) _FOREACH_129(_fn, __VA_ARGS__)
#define _FOREACH_131(_fn, x, ...) _fn(x) _FOREACH_130(_fn, __VA_ARGS__)
#define _FOREACH_132(_fn, x, ...) _fn(x) _FOREACH_131(_fn, __VA_ARGS__)
#define _FOREACH_133(_fn, x, ...) _fn(x) _FOREACH_132(_fn, __VA_ARGS__)
#define _FOREACH_134(_fn, x, ...) _fn(x) _FOREACH_133(_fn, __VA_ARGS__)
#define _FOREACH_135(_fn, x, ...) _fn(x) _FOREACH_134(_fn, __VA_ARGS__)
#define _FOREACH_136(_fn, x, ...) _fn(x) _FOREACH_135(_fn, __VA_ARGS__)
#define _FOREACH_137(_fn, x, ...) _fn(x) _FOREACH_136(_fn, __VA_ARGS__)
#define _FOREACH_138(_fn, x, ...) _fn(x) _FOREACH_137(_fn, __VA_ARGS__)
#define _FOREACH_139(_fn, x, ...) _fn(x) _FOREACH_138(_fn, __VA_ARGS__)
#define _FOREACH_140(_fn, x, ...) _fn(x) _FOREACH_139(_fn, __VA_ARGS__)
#define _FOREACH_141(_fn, x, ...) _fn(x) _FOREACH_140(_fn, __VA_ARGS__)
#define _FOREACH_142(_fn, x, ...) _fn(x) _FOREACH_141(_fn, __VA_ARGS__)
#define _FOREACH_143(_fn, x, ...) _fn(x) _FOREACH_142(_fn, __VA_ARGS__)
#define _FOREACH_144(_fn, x, ...) _fn(x) _FOREACH_143(_fn, __VA_ARGS__)
#define _FOREACH_145(_fn, x, ...) _fn(x) _FOREACH_144(_fn, __VA_ARGS__)
#define _FOREACH_146(_fn, x, ...) _fn(x) _FOREACH_145(_fn, __VA_ARGS__)
#define _FOREACH_147(_fn, x, ...) _fn(x) _FOREACH_146(_fn, __VA_ARGS__)
#define _FOREACH_148(_fn, x, ...) _fn(x) _FOREACH_147(_fn, __VA_ARGS__)
#define _FOREACH_149(_fn, x, ...) _fn(x) _FOREACH_148(_fn, __VA_ARGS__)
#define _FOREACH_150(_fn, x, ...) _fn(x) _FOREACH_149(_fn, __VA_ARGS__)
#define _FOREACH_151(_fn, x, ...) _fn(x) _FOREACH_150(_fn, __VA_ARGS__)
#define _FOREACH_152(_fn, x, ...) _fn(x) _FOREACH_151(_fn, __VA_ARGS__)
#define _FOREACH_153(_fn, x, ...) _fn(x) _FOREACH_152(_fn, __VA_ARGS__)
#define _FOREACH_154(_fn, x, ...) _fn(x) _FOREACH_153(_fn, __VA_ARGS__)
#define _FOREACH_155(_fn, x, ...) _fn(x) _FOREACH_154(_fn, __VA_ARGS__)
#define _FOREACH_156(_fn, x, ...) _fn(x) _FOREACH_155(_fn, __VA_ARGS__)
#define _FOREACH_157(_fn, x, ...) _fn(x) _FOREACH_156(_fn, __VA_ARGS__)
#define _FOREACH_158(_fn, x, ...) _fn(x) _FOREACH_157(_fn, __VA_ARGS__)
#define _FOREACH_159(_fn, x, ...) _fn(x) _FOREACH_158(_fn, __VA_ARGS__)
#define _FOREACH_160(_fn, x, ...) _fn(x) _FOREACH_159(_fn, __VA_ARGS__)
#define _FOREACH_161(_fn, x, ...) _fn(x) _FOREACH_160(_fn, __VA_ARGS__)
#define _FOREACH_162(_fn, x, ...) _fn(x) _FOREACH_161(_fn, __VA_ARGS__)
#define _FOREACH_163(_fn, x, ...) _fn(x) _FOREACH_162(_fn, __VA_ARGS__)
#define _FOREACH_164(_fn, x, ...) _fn(x) _FOREACH_163(_fn, __VA_ARGS__)
#define _FOREACH_165(_fn, x, ...) _fn(x) _FOREACH_164(_fn, __VA_ARGS__)
#define _FOREACH_166(_fn, x, ...) _fn(x) _FOREACH_165(_fn, __VA_ARGS__)
#define _FOREACH_167(_fn, x, ...) _fn(x) _FOREACH_166(_fn, __VA_ARGS__)
#define _FOREACH_168(_fn, x, ...) _fn(x) _FOREACH_167(_fn, __VA_ARGS__)
#define _FOREACH_169(_fn, x, ...) _fn(x) _FOREACH_168(_fn, __VA_ARGS__)
#define _FOREACH_170(_fn, x, ...) _fn(x) _FOREACH_169(_fn, __VA_ARGS__)
#define _FOREACH_171(_fn, x, ...) _fn(x) _FOREACH_170(_fn, __VA_ARGS__)
#define _FOREACH_172(_fn, x, ...) _fn(x) _FOREACH_171(_fn, __VA_ARGS__)
#define _FOREACH_173(_fn, x, ...) _fn(x) _FOREACH_172(_fn, __VA_ARGS__)
#define _FOREACH_174(_fn, x, ...) _fn(x) _FOREACH_173(_fn, __VA_ARGS__)
#define _FOREACH_175(_fn, x, ...) _fn(x) _FOREACH_174(_fn, __VA_ARGS__)
#define _FOREACH_176(_fn, x, ...) _fn(x) _FOREACH_175(_fn, __VA_ARGS__)
#define _FOREACH_177(_fn, x, ...) _fn(x) _FOREACH_176(_fn, __VA_ARGS__)
#define _FOREACH_178(_fn, x, ...) _fn(x) _FOREACH_177(_fn, __VA_ARGS__)
#define _FOREACH_179(_fn, x, ...) _fn(x) _FOREACH_178(_fn, __VA_ARGS__)
#define _FOREACH_180(_fn, x, ...) _fn(x) _FOREACH_179(_fn, __VA_ARGS__)
#define _FOREACH_181(_fn, x, ...) _fn(x) _FOREACH_180(_fn, __VA_ARGS__)
#define _FOREACH_182(_fn, x, ...) _fn(x) _FOREACH_181(_fn, __VA_ARGS__)
#define _FOREACH_183(_fn, x, ...) _fn(x) _FOREACH_182(_fn, __VA_ARGS__)
#define _FOREACH_184(_fn, x, ...) _fn(x) _FOREACH_183(_fn, __VA_ARGS__)
#define _FOREACH_185(_fn, x, ...) _fn(x) _FOREACH_184(_fn, __VA_ARGS__)
#define _FOREACH_186(_fn, x, ...) _fn(x) _FOREACH_185(_fn, __VA_ARGS__)
#define _FOREACH_187(_fn, x, ...) _fn(x) _FOREACH_186(_fn, __VA_ARGS__)
#define _FOREACH_188(_fn, x, ...) _fn(x) _FOREACH_187(_fn, __VA_ARGS__)
#define _FOREACH_189(_fn, x, ...) _fn(x) _FOREACH_188(_fn, __VA_ARGS__)
#define _FOREACH_190(_fn, x, ...) _fn(x) _FOREACH_189(_fn, __VA_ARGS__)
#define _FOREACH_191(_fn, x, ...) _fn(x) _FOREACH_190(_fn, __VA_ARGS__)
#define _FOREACH_192(_fn, x, ...) _fn(x) _FOREACH_191(_fn, __VA_ARGS__)
#define _FOREACH_193(_fn, x, ...) _fn(x) _FOREACH_192(_fn, __VA_ARGS__)
#define _FOREACH_194(_fn, x, ...) _fn(x) _FOREACH_193(_fn, __VA_ARGS__)
#define _FOREACH_195(_fn, x, ...) _fn(x) _FOREACH_194(_fn, __VA_ARGS__)
#define _FOREACH_196(_fn, x, ...) _fn(x) _FOREACH_195(_fn, __VA_ARGS__)
#define _FOREACH_197(_fn, x, ...) _fn(x) _FOREACH_196(_fn, __VA_ARGS__)
#define _FOREACH_198(_fn, x, ...) _fn(x) _FOREACH_197(_fn, __VA_ARGS__)
#define _FOREACH_199(_fn, x, ...) _fn(x) _FOREACH_198(_fn, __VA_ARGS__)
#define _FOREACH_200(_fn, x, ...) _fn(x) _FOREACH_199(_fn, __VA_ARGS__)
#define _FOREACH_201(_fn, x, ...) _fn(x) _FOREACH_200(_fn, __VA_ARGS__)
#define _FOREACH_202(_fn, x, ...) _fn(x) _FOREACH_201(_fn, __VA_ARGS__)
#define _FOREACH_203(_fn, x, ...) _fn(x) _FOREACH_202(_fn, __VA_ARGS__)
#define _FOREACH_204(_fn, x, ...) _fn(x) _FOREACH_203(_fn, __VA_ARGS__)
#define _FOREACH_205(_fn, x, ...) _fn(x) _FOREACH_204(_fn, __VA_ARGS__)
#define _FOREACH_206(_fn, x, ...) _fn(x) _FOREACH_205(_fn, __VA_ARGS__)
#define _FOREACH_207(_fn, x, ...) _fn(x) _FOREACH_206(_fn, __VA_ARGS__)
#define _FOREACH_208(_fn, x, ...) _fn(x) _FOREACH_207(_fn, __VA_ARGS__)
#define _FOREACH_209(_fn, x, ...) _fn(x) _FOREACH_208(_fn, __VA_ARGS__)
#define _FOREACH_210(_fn, x, ...) _fn(x) _FOREACH_209(_fn, __VA_ARGS__)
#define _FOREACH_211(_fn, x, ...) _fn(x) _FOREACH_210(_fn, __VA_ARGS__)
#define _FOREACH_212(_fn, x, ...) _fn(x) _FOREACH_211(_fn, __VA_ARGS__)
#define _FOREACH_213(_fn, x, ...) _fn(x) _FOREACH_212(_fn, __VA_ARGS__)
#define _FOREACH_214(_fn, x, ...) _fn(x) _FOREACH_213(_fn, __VA_ARGS__)
#define _FOREACH_215(_fn, x, ...) _fn(x) _FOREACH_214(_fn, __VA_ARGS__)
#define _FOREACH_216(_fn, x, ...) _fn(x) _FOREACH_215(_fn, __VA_ARGS__)
#define _FOREACH_217(_fn, x, ...) _fn(x) _FOREACH_216(_fn, __VA_ARGS__)
#define _FOREACH_218(_fn, x, ...) _fn(x) _FOREACH_217(_fn, __VA_ARGS__)
#define _FOREACH_219(_fn, x, ...) _fn(x) _FOREACH_218(_fn, __VA_ARGS__)
#define _FOREACH_220(_fn, x, ...) _fn(x) _FOREACH_219(_fn, __VA_ARGS__)
#define _FOREACH_221(_fn, x, ...) _fn(x) _FOREACH_220(_fn, __VA_ARGS__)
#define _FOREACH_222(_fn, x, ...) _fn(x) _FOREACH_221(_fn, __VA_ARGS__)
#define _FOREACH_223(_fn, x, ...) _fn(x) _FOREACH_222(_fn, __VA_ARGS__)
#define _FOREACH_224(_fn, x, ...) _fn(x) _FOREACH_223(_fn, __VA_ARGS__)
#define _FOREACH_225(_fn, x, ...) _fn(x) _FOREACH_224(_fn, __VA_ARGS__)
#define _FOREACH_226(_fn, x, ...) _fn(x) _FOREACH_225(_fn, __VA_ARGS__)
#define _FOREACH_227(_fn, x, ...) _fn(x) _FOREACH_226(_fn, __VA_ARGS__)
#define _FOREACH_228(_fn, x, ...) _fn(x) _FOREACH_227(_fn, __VA_ARGS__)
#define _FOREACH_229(_fn, x, ...) _fn(x) _FOREACH_228(_fn, __VA_ARGS__)
#define _FOREACH_230(_fn, x, ...) _fn(x) _FOREACH_229(_fn, __VA_ARGS__)
#define _FOREACH_231(_fn, x, ...) _fn(x) _FOREACH_230(_fn, __VA_ARGS__)
#define _FOREACH_232(_fn, x, ...) _fn(x) _FOREACH_231(_fn, __VA_ARGS__)
#define _FOREACH_233(_fn, x, ...) _fn(x) _FOREACH_232(_fn, __VA_ARGS__)
#define _FOREACH_234(_fn, x, ...) _fn(x) _FOREACH_233(_fn, __VA_ARGS__)
#define _FOREACH_235(_fn, x, ...) _fn(x) _FOREACH_234(_fn, __VA_ARGS__)
#define _FOREACH_236(_fn, x, ...) _fn(x) _FOREACH_235(_fn, __VA_ARGS__)
#define _FOREACH_237(_fn, x, ...) _fn(x) _FOREACH_236(_fn, __VA_ARGS__)
#define _FOREACH_238(_fn, x, ...) _fn(x) _FOREACH_237(_fn, __VA_ARGS__)
#define _FOREACH_239(_fn, x, ...) _fn(x) _FOREACH_238(_fn, __VA_ARGS__)
#define _FOREACH_240(_fn, x, ...) _fn(x) _FOREACH_239(_fn, __VA_ARGS__)
#define _FOREACH_241(_fn, x, ...) _fn(x) _FOREACH_240(_fn, __VA_ARGS__)
#define _FOREACH_242(_fn, x, ...) _fn(x) _FOREACH_241(_fn, __VA_ARGS__)
#define _FOREACH_243(_fn, x, ...) _fn(x) _FOREACH_242(_fn, __VA_ARGS__)
#define _FOREACH_244(_fn, x, ...) _fn(x) _FOREACH_243(_fn, __VA_ARGS__)
#define _FOREACH_245(_fn, x, ...) _fn(x) _FOREACH_244(_fn, __VA_ARGS__)
#define _FOREACH_246(_fn, x, ...) _fn(x) _FOREACH_245(_fn, __VA_ARGS__)
#define _FOREACH_247(_fn, x, ...) _fn(x) _FOREACH_246(_fn, __VA_ARGS__)
#define _FOREACH_248(_fn, x, ...) _fn(x) _FOREACH_247(_fn, __VA_ARGS__)
#define _FOREACH_249(_fn, x, ...) _fn(x) _FOREACH_248(_fn, __VA_ARGS__)
#define _FOREACH_250(_fn, x, ...) _fn(x) _FOREACH_249(_fn, __VA_ARGS__)

/******************************** ENUM MACROS *********************************/

/* Macros that define enums both as strings and as actual enums.  To use these,
   define an enum in the following form:

   #define FOREACH_SOME_ENUM(RESULT)                \
       RESULT(ENUM_VAL1)                            \
       RESULT(ENUM_VAL2)                            \
       RESULT(ENUM_VAL3)                            \
       ...

    Then,
 */

#define GENERATE_ENUM(ENUM) ENUM,
#define _GENERATE_STRING(STRING) #STRING,

/** This macro will define both an enum and a list of strings that reflect the
    names of the enumeration.

    Usage: 
    ```
    REFLECT_ENUM(toks,
        TOKEN1,
        TOKEN2,
        TOKEN3,
        TOKEN4,
        TOKEN5)
    ```

    The string array will be `const char *g_reflected_toks[5]`

    NOTE: currently only 250 enumeration values may be reflected.  If more are
    needed, then the N_ARG and FOREACH macros (defined in this file) need to beq
    re-generated.  I made elisp functions to automatically generate these
    values.  In emacs, do `C-x C-e` on each function.  After deleting the old
    code, run the interactive functions.  They will insert the code where the
    cursor is at.
*/
#define REFLECT_ENUM(name, ...) \
    enum name { __VA_ARGS__ }; \
    const char *g_reflected_##name[] = {FOREACH(_GENERATE_STRING, __VA_ARGS__)};

#endif
