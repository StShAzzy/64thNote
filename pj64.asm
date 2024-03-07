.686
.mmx
.model flat

DATA SEGMENT USE32

fpucontrol:
	DW 0000H

DATA ENDS


TEXT SEGMENT USE32

PUBLIC _fpuRestoreControl
_fpuRestoreControl:
	fldcw word ptr [fpucontrol]
	xor eax, eax
	mov ax, word ptr [fpucontrol]
	ret

PUBLIC _fpuSaveControl
_fpuSaveControl:
	push ebp
	mov ebp, esp
	fnstcw word ptr [ebp-2h]
	cmp dword ptr [ebp+08h], 1
	jz fpuSaveCtrlN
	xor eax, eax
    mov ax, word ptr [ebp-2h]
    and word ptr [ebp-2h], 0f3ffh
    fldcw word ptr [ebp-2h]
fpuSaveCtrlN:
	mov word ptr [fpucontrol], ax
    pop ebp
	ret


PUBLIC __DetectCpuSpecs
__DetectCpuSpecs:
	push edx
	mov eax, 1
	cpuid
	pop edx
	ret

PUBLIC __Emms
__Emms:
	emms
	ret

PUBLIC __Int3
__Int3:
	int 3
	ret


TEXT ENDS

END
