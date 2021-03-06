
;--- regression test
;--- code generation
;--- MOVD/MOVQ in 64-bit

ifdef __JWASM__
	.x64
	.model flat
endif

	.data

vq	dq 123456789ABCDEF0h
vd	dd 12345678h

	.code

@dd equ <dword ptr>
@dq equ <qword ptr>

;--- MOVD
;--- with MOVD, one can transfer content from R32/R64/M32
;--- to MMX/XMM or from MMX/XMM to R32/R64/M32

	movd mm1, eax         ;mm,  r32
	movd xmm1, eax        ;xmm, r32
	movd mm1, r8d         ;mm,  r32
	movd xmm1, r8d        ;xmm, r32
	movd mm1, rax         ;mm,  r64
	movd xmm1, rax        ;xmm, r64
	movd mm1, r8          ;mm,  r64
	movd xmm1, r8         ;xmm, r64

	movd eax, mm1         ;r32, mm
	movd eax, xmm1        ;r32, xmm
	movd r8d, mm1         ;r32, mm
	movd r8d, xmm1        ;r32, xmm
	movd rax, mm1         ;r64, mm
	movd rax, xmm1        ;r64, xmm
	movd r8, mm1          ;r64, mm
	movd r8, xmm1         ;r64, xmm

	movd mm1, vd          ;mm,  mem32
	movd xmm1, vd         ;xmm, mem32
	movd mm1, @dd [ebx]   ;mm,  mem32
	movd xmm1, @dd [ebx]  ;xmm, mem32
	movd mm1, @dd [r8d]   ;mm,  mem32
	movd xmm1, @dd [r8d]  ;xmm, mem32
	movd mm1, @dd [rax]   ;mm,  mem32
	movd xmm1, @dd [rax]  ;xmm, mem32
	movd mm1, @dd [r8]    ;mm,  mem32
	movd xmm1, @dd [r8]   ;xmm, mem32

	movd vd, mm1          ;mem32, mm
	movd vd, xmm1         ;mem32, xmm
	movd @dd [ebx], mm1   ;mem32, mm
	movd @dd [ebx], xmm1  ;mem32, xmm
	movd @dd [r8d], mm1   ;mem32, mm
	movd @dd [r8d], xmm1  ;mem32, xmm
	movd @dd [rax], mm1   ;mem32, mm
	movd @dd [rax], xmm1  ;mem32, xmm
	movd @dd [r8], mm1    ;mem32, mm
	movd @dd [r8], xmm1   ;mem32, xmm

;--- MOVQ
;--- with MOVQ, one can transfer content to/from MMX/XMM/M64 to MMX/XMM/M64

	movq mm1, mm0         ;mm,  mm
	movq xmm1, xmm0       ;xmm, xmm

	movq mm0, vq          ;mm,  mem64
	movq xmm1, vq         ;xmm, mem64
	movq mm1, @dq [ebx]   ;mm,  mem64
	movq xmm1, @dq [ebx]  ;xmm, mem64
	movq mm1, [ebx]       ;mm,  mem64
	movq xmm1, @dq [ebx]  ;xmm, mem64
	movq mm1, [r8d]       ;mm,  mem64
	movq xmm1, @dq [r8d]  ;xmm, mem64
	movq mm1, [rax]       ;mm,  mem64
	movq xmm1, @dq [rax]  ;xmm, mem64
	movq mm1, [r8]        ;mm,  mem64
	movq xmm1, @dq [r8]   ;xmm, mem64

	movq vq, mm0          ;mem64, mm
	movq vq, xmm0         ;mem64, xmm
	movq @dq [ebx], mm0   ;mem64, mm
	movq @dq [ebx], xmm0  ;mem64, xmm
	movq [ebx], mm1       ;mem64, mm
	movq @dq [ebx], xmm1  ;mem64, xmm
	movq [r8d], mm0       ;mem64, mm
	movq @dq [r8d], xmm0  ;mem64, xmm
	movq [rax], mm2       ;mem64, mm
	movq @dq [rax], xmm2  ;mem64, xmm
	movq [r8], mm2        ;mem64, mm
	movq @dq [r8], xmm2   ;mem64, xmm

;	movq rax, mm1         ;r64, mm
;	movq rax, xmm1        ;r64, xmm

	ret

	end
