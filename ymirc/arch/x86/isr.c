#include "isr.h"

/** Define ISR function for the given vector. */
#define DEFINE_ISR(vector)                                                    \
  __attribute__((naked)) void isr_##vector(void) {                            \
    /** Clear the interrupt flag. */                                          \
    __asm__ volatile("cli");                                                  \
    /** If the interrupt does not provide an error code, push a dummy one. */ \
    if ((vector) != 8 && !((vector) >= 10 && (vector) <= 14) &&               \
        (vector) != 17) {                                                     \
      __asm__ volatile("pushq $0");                                           \
    }                                                                         \
    /** Push the vector. */                                                   \
    __asm__ volatile("pushq %0" : : "n"(vector));                             \
    /** Jump to the common ISR. */                                            \
    __asm__ volatile("jmp isr_common");                                       \
  }

/** Common stub for all ISR, that all the ISRs will use. This function assumes
 * that `Context` is saved at the top of the stack except for general-purpose
 * registers. */
__attribute__((naked)) void isr_common() {
  // Save the general-purpose registers.
  __asm__ volatile(
      "pushq %rax\n\t"
      "pushq %rcx\n\t"
      "pushq %rdx\n\t"
      "pushq %rbx\n\t"
      "pushq %rsp\n\t"
      "pushq %rbp\n\t"
      "pushq %rsi\n\t"
      "pushq %rdi\n\t"
      "pushq %r15\n\t"
      "pushq %r14\n\t"
      "pushq %r13\n\t"
      "pushq %r12\n\t"
      "pushq %r11\n\t"
      "pushq %r10\n\t"
      "pushq %r9\n\t"
      "pushq %r8");

  // Push the context and call the handler.
  __asm__ volatile(
      "pushq %rsp\n\t"
      "popq %rdi\n\t"
      // Align stack to 16 bytes.
      "pushq %rsp\n\t"
      "pushq (%rsp)\n\t"
      "andq $-0x10, %rsp\n\t"
      // Call the dispatcher.
      "call itr_dispatch\n\t"
      // Restore the stack.
      "movq 8(%rsp), %rsp");

  // Remove general-purpose registers, error code, and vector from the stack.
  __asm__ volatile(
      "popq %r8\n\t"
      "popq %r9\n\t"
      "popq %r10\n\t"
      "popq %r11\n\t"
      "popq %r12\n\t"
      "popq %r13\n\t"
      "popq %r14\n\t"
      "popq %r15\n\t"
      "popq %rdi\n\t"
      "popq %rsi\n\t"
      "popq %rbp\n\t"
      "popq %rsp\n\t"
      "popq %rbx\n\t"
      "popq %rdx\n\t"
      "popq %rcx\n\t"
      "popq %rax\n\t"
      "add $0x10, %rsp\n\t"
      "iretq");
}

DEFINE_ISR(0)
DEFINE_ISR(1)
DEFINE_ISR(2)
DEFINE_ISR(3)
DEFINE_ISR(4)
DEFINE_ISR(5)
DEFINE_ISR(6)
DEFINE_ISR(7)
DEFINE_ISR(8)
DEFINE_ISR(9)
DEFINE_ISR(10)
DEFINE_ISR(11)
DEFINE_ISR(12)
DEFINE_ISR(13)
DEFINE_ISR(14)
DEFINE_ISR(15)
DEFINE_ISR(16)
DEFINE_ISR(17)
DEFINE_ISR(18)
DEFINE_ISR(19)
DEFINE_ISR(20)
DEFINE_ISR(21)
DEFINE_ISR(22)
DEFINE_ISR(23)
DEFINE_ISR(24)
DEFINE_ISR(25)
DEFINE_ISR(26)
DEFINE_ISR(27)
DEFINE_ISR(28)
DEFINE_ISR(29)
DEFINE_ISR(30)
DEFINE_ISR(31)
DEFINE_ISR(32)
DEFINE_ISR(33)
DEFINE_ISR(34)
DEFINE_ISR(35)
DEFINE_ISR(36)
DEFINE_ISR(37)
DEFINE_ISR(38)
DEFINE_ISR(39)
DEFINE_ISR(40)
DEFINE_ISR(41)
DEFINE_ISR(42)
DEFINE_ISR(43)
DEFINE_ISR(44)
DEFINE_ISR(45)
DEFINE_ISR(46)
DEFINE_ISR(47)
DEFINE_ISR(48)
DEFINE_ISR(49)
DEFINE_ISR(50)
DEFINE_ISR(51)
DEFINE_ISR(52)
DEFINE_ISR(53)
DEFINE_ISR(54)
DEFINE_ISR(55)
DEFINE_ISR(56)
DEFINE_ISR(57)
DEFINE_ISR(58)
DEFINE_ISR(59)
DEFINE_ISR(60)
DEFINE_ISR(61)
DEFINE_ISR(62)
DEFINE_ISR(63)
DEFINE_ISR(64)
DEFINE_ISR(65)
DEFINE_ISR(66)
DEFINE_ISR(67)
DEFINE_ISR(68)
DEFINE_ISR(69)
DEFINE_ISR(70)
DEFINE_ISR(71)
DEFINE_ISR(72)
DEFINE_ISR(73)
DEFINE_ISR(74)
DEFINE_ISR(75)
DEFINE_ISR(76)
DEFINE_ISR(77)
DEFINE_ISR(78)
DEFINE_ISR(79)
DEFINE_ISR(80)
DEFINE_ISR(81)
DEFINE_ISR(82)
DEFINE_ISR(83)
DEFINE_ISR(84)
DEFINE_ISR(85)
DEFINE_ISR(86)
DEFINE_ISR(87)
DEFINE_ISR(88)
DEFINE_ISR(89)
DEFINE_ISR(90)
DEFINE_ISR(91)
DEFINE_ISR(92)
DEFINE_ISR(93)
DEFINE_ISR(94)
DEFINE_ISR(95)
DEFINE_ISR(96)
DEFINE_ISR(97)
DEFINE_ISR(98)
DEFINE_ISR(99)
DEFINE_ISR(100)
DEFINE_ISR(101)
DEFINE_ISR(102)
DEFINE_ISR(103)
DEFINE_ISR(104)
DEFINE_ISR(105)
DEFINE_ISR(106)
DEFINE_ISR(107)
DEFINE_ISR(108)
DEFINE_ISR(109)
DEFINE_ISR(110)
DEFINE_ISR(111)
DEFINE_ISR(112)
DEFINE_ISR(113)
DEFINE_ISR(114)
DEFINE_ISR(115)
DEFINE_ISR(116)
DEFINE_ISR(117)
DEFINE_ISR(118)
DEFINE_ISR(119)
DEFINE_ISR(120)
DEFINE_ISR(121)
DEFINE_ISR(122)
DEFINE_ISR(123)
DEFINE_ISR(124)
DEFINE_ISR(125)
DEFINE_ISR(126)
DEFINE_ISR(127)
DEFINE_ISR(128)
DEFINE_ISR(129)
DEFINE_ISR(130)
DEFINE_ISR(131)
DEFINE_ISR(132)
DEFINE_ISR(133)
DEFINE_ISR(134)
DEFINE_ISR(135)
DEFINE_ISR(136)
DEFINE_ISR(137)
DEFINE_ISR(138)
DEFINE_ISR(139)
DEFINE_ISR(140)
DEFINE_ISR(141)
DEFINE_ISR(142)
DEFINE_ISR(143)
DEFINE_ISR(144)
DEFINE_ISR(145)
DEFINE_ISR(146)
DEFINE_ISR(147)
DEFINE_ISR(148)
DEFINE_ISR(149)
DEFINE_ISR(150)
DEFINE_ISR(151)
DEFINE_ISR(152)
DEFINE_ISR(153)
DEFINE_ISR(154)
DEFINE_ISR(155)
DEFINE_ISR(156)
DEFINE_ISR(157)
DEFINE_ISR(158)
DEFINE_ISR(159)
DEFINE_ISR(160)
DEFINE_ISR(161)
DEFINE_ISR(162)
DEFINE_ISR(163)
DEFINE_ISR(164)
DEFINE_ISR(165)
DEFINE_ISR(166)
DEFINE_ISR(167)
DEFINE_ISR(168)
DEFINE_ISR(169)
DEFINE_ISR(170)
DEFINE_ISR(171)
DEFINE_ISR(172)
DEFINE_ISR(173)
DEFINE_ISR(174)
DEFINE_ISR(175)
DEFINE_ISR(176)
DEFINE_ISR(177)
DEFINE_ISR(178)
DEFINE_ISR(179)
DEFINE_ISR(180)
DEFINE_ISR(181)
DEFINE_ISR(182)
DEFINE_ISR(183)
DEFINE_ISR(184)
DEFINE_ISR(185)
DEFINE_ISR(186)
DEFINE_ISR(187)
DEFINE_ISR(188)
DEFINE_ISR(189)
DEFINE_ISR(190)
DEFINE_ISR(191)
DEFINE_ISR(192)
DEFINE_ISR(193)
DEFINE_ISR(194)
DEFINE_ISR(195)
DEFINE_ISR(196)
DEFINE_ISR(197)
DEFINE_ISR(198)
DEFINE_ISR(199)
DEFINE_ISR(200)
DEFINE_ISR(201)
DEFINE_ISR(202)
DEFINE_ISR(203)
DEFINE_ISR(204)
DEFINE_ISR(205)
DEFINE_ISR(206)
DEFINE_ISR(207)
DEFINE_ISR(208)
DEFINE_ISR(209)
DEFINE_ISR(210)
DEFINE_ISR(211)
DEFINE_ISR(212)
DEFINE_ISR(213)
DEFINE_ISR(214)
DEFINE_ISR(215)
DEFINE_ISR(216)
DEFINE_ISR(217)
DEFINE_ISR(218)
DEFINE_ISR(219)
DEFINE_ISR(220)
DEFINE_ISR(221)
DEFINE_ISR(222)
DEFINE_ISR(223)
DEFINE_ISR(224)
DEFINE_ISR(225)
DEFINE_ISR(226)
DEFINE_ISR(227)
DEFINE_ISR(228)
DEFINE_ISR(229)
DEFINE_ISR(230)
DEFINE_ISR(231)
DEFINE_ISR(232)
DEFINE_ISR(233)
DEFINE_ISR(234)
DEFINE_ISR(235)
DEFINE_ISR(236)
DEFINE_ISR(237)
DEFINE_ISR(238)
DEFINE_ISR(239)
DEFINE_ISR(240)
DEFINE_ISR(241)
DEFINE_ISR(242)
DEFINE_ISR(243)
DEFINE_ISR(244)
DEFINE_ISR(245)
DEFINE_ISR(246)
DEFINE_ISR(247)
DEFINE_ISR(248)
DEFINE_ISR(249)
DEFINE_ISR(250)
DEFINE_ISR(251)
DEFINE_ISR(252)
DEFINE_ISR(253)
DEFINE_ISR(254)
DEFINE_ISR(255)

Isr isr_table[256] = {
    isr_0,   isr_1,   isr_2,   isr_3,   isr_4,   isr_5,   isr_6,   isr_7,
    isr_8,   isr_9,   isr_10,  isr_11,  isr_12,  isr_13,  isr_14,  isr_15,
    isr_16,  isr_17,  isr_18,  isr_19,  isr_20,  isr_21,  isr_22,  isr_23,
    isr_24,  isr_25,  isr_26,  isr_27,  isr_28,  isr_29,  isr_30,  isr_31,
    isr_32,  isr_33,  isr_34,  isr_35,  isr_36,  isr_37,  isr_38,  isr_39,
    isr_40,  isr_41,  isr_42,  isr_43,  isr_44,  isr_45,  isr_46,  isr_47,
    isr_48,  isr_49,  isr_50,  isr_51,  isr_52,  isr_53,  isr_54,  isr_55,
    isr_56,  isr_57,  isr_58,  isr_59,  isr_60,  isr_61,  isr_62,  isr_63,
    isr_64,  isr_65,  isr_66,  isr_67,  isr_68,  isr_69,  isr_70,  isr_71,
    isr_72,  isr_73,  isr_74,  isr_75,  isr_76,  isr_77,  isr_78,  isr_79,
    isr_80,  isr_81,  isr_82,  isr_83,  isr_84,  isr_85,  isr_86,  isr_87,
    isr_88,  isr_89,  isr_90,  isr_91,  isr_92,  isr_93,  isr_94,  isr_95,
    isr_96,  isr_97,  isr_98,  isr_99,  isr_100, isr_101, isr_102, isr_103,
    isr_104, isr_105, isr_106, isr_107, isr_108, isr_109, isr_110, isr_111,
    isr_112, isr_113, isr_114, isr_115, isr_116, isr_117, isr_118, isr_119,
    isr_120, isr_121, isr_122, isr_123, isr_124, isr_125, isr_126, isr_127,
    isr_128, isr_129, isr_130, isr_131, isr_132, isr_133, isr_134, isr_135,
    isr_136, isr_137, isr_138, isr_139, isr_140, isr_141, isr_142, isr_143,
    isr_144, isr_145, isr_146, isr_147, isr_148, isr_149, isr_150, isr_151,
    isr_152, isr_153, isr_154, isr_155, isr_156, isr_157, isr_158, isr_159,
    isr_160, isr_161, isr_162, isr_163, isr_164, isr_165, isr_166, isr_167,
    isr_168, isr_169, isr_170, isr_171, isr_172, isr_173, isr_174, isr_175,
    isr_176, isr_177, isr_178, isr_179, isr_180, isr_181, isr_182, isr_183,
    isr_184, isr_185, isr_186, isr_187, isr_188, isr_189, isr_190, isr_191,
    isr_192, isr_193, isr_194, isr_195, isr_196, isr_197, isr_198, isr_199,
    isr_200, isr_201, isr_202, isr_203, isr_204, isr_205, isr_206, isr_207,
    isr_208, isr_209, isr_210, isr_211, isr_212, isr_213, isr_214, isr_215,
    isr_216, isr_217, isr_218, isr_219, isr_220, isr_221, isr_222, isr_223,
    isr_224, isr_225, isr_226, isr_227, isr_228, isr_229, isr_230, isr_231,
    isr_232, isr_233, isr_234, isr_235, isr_236, isr_237, isr_238, isr_239,
    isr_240, isr_241, isr_242, isr_243, isr_244, isr_245, isr_246, isr_247,
    isr_248, isr_249, isr_250, isr_251, isr_252, isr_253, isr_254, isr_255};
