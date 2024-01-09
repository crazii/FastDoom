;
; DESCRIPTION: Assembly texture mapping routines for VESA modes
;

BITS 32
%include "macros.inc"

%ifdef MODE_VBE2_DIRECT
%include "defs.inc"

extern _destview
extern _colormaps

;============================================================================
; unwound vertical scaling code
;
; eax   light table pointer, 0 lowbyte overwritten
; ebx   all 0, low byte overwritten
; ecx   fractional step value
; edx   fractional scale value
; esi   start of source pixels
; edi   bottom pixel in screenbuffer to blit into
;
; ebx should be set to 0 0 0 dh to feed the pipeline
;
; The graphics wrap vertically at 128 pixels
;============================================================================

BEGIN_DATA_SECTION

%macro SCALEDEFINE 1
  dd vscale%1
%endmacro

align 4

scalecalls:
  %assign LINE 0
  %rep SCREENHEIGHT+1
    SCALEDEFINE LINE
  %assign LINE LINE+1
  %endrep

;============================================================================

BEGIN_CODE_SECTION

;============================================================================
;
; R_DrawColumn
;
;============================================================================

CODE_SYM_DEF R_DrawFuzzColumnFlatLowVBE2
	push		edi
	push		ebx
	push		ecx
	push		edx
	push		esi
	push		ebp

  mov  ebp,[_dc_yh]
  mov  eax,[_dc_yl]
  MulScreenWidthStart edi, ebp
  sub  ebp,eax ; ebp = pixel count
  js   near .done

  MulScreenWidthEnd edi
  mov  eax,[_dc_x]
  xor ecx,ecx
  lea  edi,[edi+eax*2]
  mov eax,[_colormaps]
  add  edi,[_destview]
  add eax,0x600

  jmp  [scalecalls+4+ebp*4]

.done:
	pop		ebp
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  pop		edi
  ret
; R_DrawColumnVBE2 ends

;============ HIGH DETAIL ============

%macro SCALELABEL 1
  vscale%1
%endmacro

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
  SCALELABEL LINE:

	mov   al,[edi-(LINE-1)*SCREENWIDTH]
	mov		cl,[eax]
  mov   ch,cl
  mov		[edi-(LINE-1)*SCREENWIDTH],cx

  %assign LINE LINE-1
%endrep

vscale1:
  mov   al,[edi-(LINE-1)*SCREENWIDTH]
	pop	ebp
	mov		cl,[eax]
  pop	esi
  mov   ch,cl
  pop	edx
  mov		[edi-(LINE-1)*SCREENWIDTH],cx

vscale0:
	pop	ecx
	pop	ebx
  pop	edi
  ret

%endif
