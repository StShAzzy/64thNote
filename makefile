# Microsoft Developer Studio Generated NMAKE File, Based on Project64.dsp
!IF "$(CFG)" == ""
CFG=Project64 - Win32 Release External
!MESSAGE No configuration specified. Defaulting to Project64 - Win32 Release External.
!ENDIF

!IF "$(CFG)" != "Project64 - Win32 Release External"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "Project64.mak" CFG="Project64 - Win32 Release External"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "Project64 - Win32 Release External" (based on "Win32 (x86) Application")
!MESSAGE
!ERROR An invalid configuration is specified.
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL=nul
!ENDIF

OUTDIR=.\Release_External
INTDIR=.\Release_External
# Begin Custom Macros
OutDir=.\Release_External
# End Custom Macros

ALL : "c:\progra~1\winamp5\plugins\in_usf.dll" "$(OUTDIR)\Project64.bsc"


CLEAN :
	-@erase "$(INTDIR)\audio hle main.obj"
	-@erase "$(INTDIR)\audio hle main.sbr"
	-@erase "$(INTDIR)\audio ucode1.obj"
	-@erase "$(INTDIR)\audio ucode1.sbr"
	-@erase "$(INTDIR)\audio ucode2.obj"
	-@erase "$(INTDIR)\audio ucode2.sbr"
	-@erase "$(INTDIR)\audio ucode3.obj"
	-@erase "$(INTDIR)\audio ucode3.sbr"
	-@erase "$(INTDIR)\audio ucode3mp3.obj"
	-@erase "$(INTDIR)\audio ucode3mp3.sbr"
	-@erase "$(INTDIR)\Audio.obj"
	-@erase "$(INTDIR)\Audio.sbr"
	-@erase "$(INTDIR)\CPU.obj"
	-@erase "$(INTDIR)\CPU.sbr"
	-@erase "$(INTDIR)\DMA.obj"
	-@erase "$(INTDIR)\DMA.sbr"
	-@erase "$(INTDIR)\Exception.obj"
	-@erase "$(INTDIR)\Exception.sbr"
	-@erase "$(INTDIR)\Interpreter CPU.obj"
	-@erase "$(INTDIR)\Interpreter CPU.sbr"
	-@erase "$(INTDIR)\Interpreter Ops.obj"
	-@erase "$(INTDIR)\Interpreter Ops.sbr"
	-@erase "$(INTDIR)\lang.obj"
	-@erase "$(INTDIR)\lang.sbr"
	-@erase "$(INTDIR)\Main.obj"
	-@erase "$(INTDIR)\Main.sbr"
	-@erase "$(INTDIR)\Memory.obj"
	-@erase "$(INTDIR)\Memory.sbr"
	-@erase "$(INTDIR)\pif.obj"
	-@erase "$(INTDIR)\pif.sbr"
	-@erase "$(INTDIR)\Plugin.obj"
	-@erase "$(INTDIR)\Plugin.sbr"
	-@erase "$(INTDIR)\psftag.obj"
	-@erase "$(INTDIR)\psftag.sbr"
	-@erase "$(INTDIR)\Recompiler CPU.obj"
	-@erase "$(INTDIR)\Recompiler CPU.sbr"
	-@erase "$(INTDIR)\Recompiler Fpu Ops.obj"
	-@erase "$(INTDIR)\Recompiler Fpu Ops.sbr"
	-@erase "$(INTDIR)\Recompiler Ops.obj"
	-@erase "$(INTDIR)\Recompiler Ops.sbr"
	-@erase "$(INTDIR)\Registers.obj"
	-@erase "$(INTDIR)\Registers.sbr"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\rom.obj"
	-@erase "$(INTDIR)\rom.sbr"
	-@erase "$(INTDIR)\RSP Cpu.obj"
	-@erase "$(INTDIR)\RSP Cpu.sbr"
	-@erase "$(INTDIR)\RSP dma.obj"
	-@erase "$(INTDIR)\RSP dma.sbr"
	-@erase "$(INTDIR)\RSP Interpreter CPU.obj"
	-@erase "$(INTDIR)\RSP Interpreter CPU.sbr"
	-@erase "$(INTDIR)\RSP Interpreter Ops.obj"
	-@erase "$(INTDIR)\RSP Interpreter Ops.sbr"
	-@erase "$(INTDIR)\RSP Main.obj"
	-@erase "$(INTDIR)\RSP Main.sbr"
	-@erase "$(INTDIR)\RSP memory.obj"
	-@erase "$(INTDIR)\RSP memory.sbr"
	-@erase "$(INTDIR)\RSP Mmx.obj"
	-@erase "$(INTDIR)\RSP Mmx.sbr"
	-@erase "$(INTDIR)\RSP Recompiler Analysis.obj"
	-@erase "$(INTDIR)\RSP Recompiler Analysis.sbr"
	-@erase "$(INTDIR)\RSP Recompiler CPU.obj"
	-@erase "$(INTDIR)\RSP Recompiler CPU.sbr"
	-@erase "$(INTDIR)\RSP Recompiler Ops.obj"
	-@erase "$(INTDIR)\RSP Recompiler Ops.sbr"
	-@erase "$(INTDIR)\RSP Recompiler Sections.obj"
	-@erase "$(INTDIR)\RSP Recompiler Sections.sbr"
	-@erase "$(INTDIR)\RSP Register.obj"
	-@erase "$(INTDIR)\RSP Register.sbr"
	-@erase "$(INTDIR)\RSP Sse.obj"
	-@erase "$(INTDIR)\RSP Sse.sbr"
	-@erase "$(INTDIR)\RSP X86.obj"
	-@erase "$(INTDIR)\RSP X86.sbr"
	-@erase "$(INTDIR)\TLB.obj"
	-@erase "$(INTDIR)\TLB.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\WAmain.obj"
	-@erase "$(INTDIR)\WAmain.sbr"
	-@erase "$(INTDIR)\x86 fpu.obj"
	-@erase "$(INTDIR)\x86 fpu.sbr"
	-@erase "$(INTDIR)\X86.obj"
	-@erase "$(INTDIR)\X86.sbr"
	-@erase "$(OUTDIR)\in_usf.exp"
	-@erase "$(OUTDIR)\in_usf.lib"
	-@erase "$(OUTDIR)\Project64.bsc"
	-@erase "c:\progra~1\winamp5\plugins\in_usf.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G6 /D "EXTERNAL_RELEASE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Project64.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
RSC_PROJ=/l 0xc09 /fo"$(INTDIR)\resource.res" /d "NDEBUG" /d "EXTERNAL_RELEASE"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Project64.bsc"
BSC32_SBRS= \
	"$(INTDIR)\lang.sbr" \
	"$(INTDIR)\Main.sbr" \
	"$(INTDIR)\psftag.sbr" \
	"$(INTDIR)\WAmain.sbr" \
	"$(INTDIR)\Audio.sbr" \
	"$(INTDIR)\CPU.sbr" \
	"$(INTDIR)\DMA.sbr" \
	"$(INTDIR)\Exception.sbr" \
	"$(INTDIR)\Interpreter CPU.sbr" \
	"$(INTDIR)\Interpreter Ops.sbr" \
	"$(INTDIR)\Memory.sbr" \
	"$(INTDIR)\pif.sbr" \
	"$(INTDIR)\Recompiler CPU.sbr" \
	"$(INTDIR)\Recompiler Fpu Ops.sbr" \
	"$(INTDIR)\Recompiler Ops.sbr" \
	"$(INTDIR)\Registers.sbr" \
	"$(INTDIR)\rom.sbr" \
	"$(INTDIR)\TLB.sbr" \
	"$(INTDIR)\Plugin.sbr" \
	"$(INTDIR)\x86 fpu.sbr" \
	"$(INTDIR)\X86.sbr" \
	"$(INTDIR)\audio hle main.sbr" \
	"$(INTDIR)\audio ucode1.sbr" \
	"$(INTDIR)\audio ucode2.sbr" \
	"$(INTDIR)\audio ucode3.sbr" \
	"$(INTDIR)\audio ucode3mp3.sbr" \
	"$(INTDIR)\RSP Cpu.sbr" \
	"$(INTDIR)\RSP dma.sbr" \
	"$(INTDIR)\RSP Interpreter CPU.sbr" \
	"$(INTDIR)\RSP Interpreter Ops.sbr" \
	"$(INTDIR)\RSP Main.sbr" \
	"$(INTDIR)\RSP memory.sbr" \
	"$(INTDIR)\RSP Mmx.sbr" \
	"$(INTDIR)\RSP Recompiler Analysis.sbr" \
	"$(INTDIR)\RSP Recompiler CPU.sbr" \
	"$(INTDIR)\RSP Recompiler Ops.sbr" \
	"$(INTDIR)\RSP Recompiler Sections.sbr" \
	"$(INTDIR)\RSP Register.sbr" \
	"$(INTDIR)\RSP Sse.sbr" \
	"$(INTDIR)\RSP X86.sbr"

"$(OUTDIR)\Project64.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\in_usf.pdb" /machine:I386 /nodefaultlib:"LIBC" /out:"c:\progra~1\winamp5\plugins\in_usf.dll" /implib:"$(OUTDIR)\in_usf.lib"
LINK32_OBJS= \
	"$(INTDIR)\lang.obj" \
	"$(INTDIR)\Main.obj" \
	"$(INTDIR)\psftag.obj" \
	"$(INTDIR)\WAmain.obj" \
	"$(INTDIR)\Audio.obj" \
	"$(INTDIR)\CPU.obj" \
	"$(INTDIR)\DMA.obj" \
	"$(INTDIR)\Exception.obj" \
	"$(INTDIR)\Interpreter CPU.obj" \
	"$(INTDIR)\Interpreter Ops.obj" \
	"$(INTDIR)\Memory.obj" \
	"$(INTDIR)\pif.obj" \
	"$(INTDIR)\Recompiler CPU.obj" \
	"$(INTDIR)\Recompiler Fpu Ops.obj" \
	"$(INTDIR)\Recompiler Ops.obj" \
	"$(INTDIR)\Registers.obj" \
	"$(INTDIR)\rom.obj" \
	"$(INTDIR)\TLB.obj" \
	"$(INTDIR)\Plugin.obj" \
	"$(INTDIR)\x86 fpu.obj" \
	"$(INTDIR)\X86.obj" \
	"$(INTDIR)\audio hle main.obj" \
	"$(INTDIR)\audio ucode1.obj" \
	"$(INTDIR)\audio ucode2.obj" \
	"$(INTDIR)\audio ucode3.obj" \
	"$(INTDIR)\audio ucode3mp3.obj" \
	"$(INTDIR)\RSP Cpu.obj" \
	"$(INTDIR)\RSP dma.obj" \
	"$(INTDIR)\RSP Interpreter CPU.obj" \
	"$(INTDIR)\RSP Interpreter Ops.obj" \
	"$(INTDIR)\RSP Main.obj" \
	"$(INTDIR)\RSP memory.obj" \
	"$(INTDIR)\RSP Mmx.obj" \
	"$(INTDIR)\RSP Recompiler Analysis.obj" \
	"$(INTDIR)\RSP Recompiler CPU.obj" \
	"$(INTDIR)\RSP Recompiler Ops.obj" \
	"$(INTDIR)\RSP Recompiler Sections.obj" \
	"$(INTDIR)\RSP Register.obj" \
	"$(INTDIR)\RSP Sse.obj" \
	"$(INTDIR)\RSP X86.obj" \
	"$(INTDIR)\resource.res"

"c:\progra~1\winamp5\plugins\in_usf.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
PostBuild_Desc=copying in_usf.ini
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release_External
# End Custom Macros

$(DS_POSTBUILD_DEP) : "c:\progra~1\winamp5\plugins\in_usf.dll" "$(OUTDIR)\Project64.bsc"
   copy in_usf.ini c:\progra~1\winamp5\plugins\in_usf.ini
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("Project64.dep")
!INCLUDE "Project64.dep"
!ELSE
#!MESSAGE Warning: cannot find "Project64.dep"
!ENDIF
!ENDIF


!IF "$(CFG)" == "Project64 - Win32 Release External"
SOURCE=.\lang.c

"$(INTDIR)\lang.obj"	"$(INTDIR)\lang.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Main.c

"$(INTDIR)\Main.obj"	"$(INTDIR)\Main.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\psftag.c

"$(INTDIR)\psftag.obj"	"$(INTDIR)\psftag.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\WAmain.c

"$(INTDIR)\WAmain.obj"	"$(INTDIR)\WAmain.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Audio.c

"$(INTDIR)\Audio.obj"	"$(INTDIR)\Audio.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CPU.c

"$(INTDIR)\CPU.obj"	"$(INTDIR)\CPU.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\DMA.c

"$(INTDIR)\DMA.obj"	"$(INTDIR)\DMA.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Exception.c

"$(INTDIR)\Exception.obj"	"$(INTDIR)\Exception.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\Interpreter CPU.c"

"$(INTDIR)\Interpreter CPU.obj"	"$(INTDIR)\Interpreter CPU.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\Interpreter Ops.c"

"$(INTDIR)\Interpreter Ops.obj"	"$(INTDIR)\Interpreter Ops.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Memory.c

"$(INTDIR)\Memory.obj"	"$(INTDIR)\Memory.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\pif.c

"$(INTDIR)\pif.obj"	"$(INTDIR)\pif.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\Recompiler CPU.c"

"$(INTDIR)\Recompiler CPU.obj"	"$(INTDIR)\Recompiler CPU.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\Recompiler Fpu Ops.c"

"$(INTDIR)\Recompiler Fpu Ops.obj"	"$(INTDIR)\Recompiler Fpu Ops.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\Recompiler Ops.c"

"$(INTDIR)\Recompiler Ops.obj"	"$(INTDIR)\Recompiler Ops.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Registers.c

"$(INTDIR)\Registers.obj"	"$(INTDIR)\Registers.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\rom.c

"$(INTDIR)\rom.obj"	"$(INTDIR)\rom.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\TLB.c

"$(INTDIR)\TLB.obj"	"$(INTDIR)\TLB.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Plugin.c
CPP_SWITCHES=/nologo /G6 /Gr /D "EXTERNAL_RELEASE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Project64.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\Plugin.obj"	"$(INTDIR)\Plugin.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


SOURCE=".\x86 fpu.c"

"$(INTDIR)\x86 fpu.obj"	"$(INTDIR)\x86 fpu.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\X86.c

"$(INTDIR)\X86.obj"	"$(INTDIR)\X86.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\audio hle main.cpp"

"$(INTDIR)\audio hle main.obj"	"$(INTDIR)\audio hle main.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\audio ucode1.cpp"

"$(INTDIR)\audio ucode1.obj"	"$(INTDIR)\audio ucode1.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\audio ucode2.cpp"

"$(INTDIR)\audio ucode2.obj"	"$(INTDIR)\audio ucode2.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\audio ucode3.cpp"

"$(INTDIR)\audio ucode3.obj"	"$(INTDIR)\audio ucode3.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\audio ucode3mp3.cpp"

"$(INTDIR)\audio ucode3mp3.obj"	"$(INTDIR)\audio ucode3mp3.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP Cpu.c"

"$(INTDIR)\RSP Cpu.obj"	"$(INTDIR)\RSP Cpu.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP dma.c"

"$(INTDIR)\RSP dma.obj"	"$(INTDIR)\RSP dma.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP Interpreter CPU.c"

"$(INTDIR)\RSP Interpreter CPU.obj"	"$(INTDIR)\RSP Interpreter CPU.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP Interpreter Ops.c"

"$(INTDIR)\RSP Interpreter Ops.obj"	"$(INTDIR)\RSP Interpreter Ops.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP Main.c"

"$(INTDIR)\RSP Main.obj"	"$(INTDIR)\RSP Main.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP memory.c"

"$(INTDIR)\RSP memory.obj"	"$(INTDIR)\RSP memory.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP Mmx.c"

"$(INTDIR)\RSP Mmx.obj"	"$(INTDIR)\RSP Mmx.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP Recompiler Analysis.c"

"$(INTDIR)\RSP Recompiler Analysis.obj"	"$(INTDIR)\RSP Recompiler Analysis.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP Recompiler CPU.c"

"$(INTDIR)\RSP Recompiler CPU.obj"	"$(INTDIR)\RSP Recompiler CPU.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP Recompiler Ops.c"

"$(INTDIR)\RSP Recompiler Ops.obj"	"$(INTDIR)\RSP Recompiler Ops.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP Recompiler Sections.c"

"$(INTDIR)\RSP Recompiler Sections.obj"	"$(INTDIR)\RSP Recompiler Sections.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP Register.c"

"$(INTDIR)\RSP Register.obj"	"$(INTDIR)\RSP Register.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP Sse.c"

"$(INTDIR)\RSP Sse.obj"	"$(INTDIR)\RSP Sse.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\RSP X86.c"

"$(INTDIR)\RSP X86.obj"	"$(INTDIR)\RSP X86.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\resource.rc

"$(INTDIR)\resource.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF

