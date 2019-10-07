##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Release
ProjectName            :=gamma
ConfigurationName      :=Release
WorkspacePath          :=/home/anton/1/cpp/2019/00armo
ProjectPath            :=/home/anton/1/cpp/2019/00armo/gamma
IntermediateDirectory  :=./Release
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=anton
Date                   :=07/10/19
CodeLitePath           :=/home/anton/.codelite
LinkerName             :=/usr/bin/g++
SharedObjectLinkerName :=/usr/bin/g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)NDEBUG 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="gamma.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  -lpthread
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)./include 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -O2 -std=c++14 -Wall $(Preprocessors)
CFLAGS   :=  -O2 -Wall $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_gamma.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_randomroutines.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_helper.cpp$(ObjectSuffix) $(IntermediateDirectory)/up_Display_ConsoleDisplay_src_display.cpp$(ObjectSuffix) $(IntermediateDirectory)/src_LFSR.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@test -d ./Release || $(MakeDirCommand) ./Release


$(IntermediateDirectory)/.d:
	@test -d ./Release || $(MakeDirCommand) ./Release

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/main.cpp$(ObjectSuffix): main.cpp $(IntermediateDirectory)/main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/anton/1/cpp/2019/00armo/gamma/main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.cpp$(DependSuffix): main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/main.cpp$(DependSuffix) -MM main.cpp

$(IntermediateDirectory)/main.cpp$(PreprocessSuffix): main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.cpp$(PreprocessSuffix) main.cpp

$(IntermediateDirectory)/src_gamma.cpp$(ObjectSuffix): src/gamma.cpp $(IntermediateDirectory)/src_gamma.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/anton/1/cpp/2019/00armo/gamma/src/gamma.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_gamma.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_gamma.cpp$(DependSuffix): src/gamma.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_gamma.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_gamma.cpp$(DependSuffix) -MM src/gamma.cpp

$(IntermediateDirectory)/src_gamma.cpp$(PreprocessSuffix): src/gamma.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_gamma.cpp$(PreprocessSuffix) src/gamma.cpp

$(IntermediateDirectory)/src_randomroutines.cpp$(ObjectSuffix): src/randomroutines.cpp $(IntermediateDirectory)/src_randomroutines.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/anton/1/cpp/2019/00armo/gamma/src/randomroutines.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_randomroutines.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_randomroutines.cpp$(DependSuffix): src/randomroutines.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_randomroutines.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_randomroutines.cpp$(DependSuffix) -MM src/randomroutines.cpp

$(IntermediateDirectory)/src_randomroutines.cpp$(PreprocessSuffix): src/randomroutines.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_randomroutines.cpp$(PreprocessSuffix) src/randomroutines.cpp

$(IntermediateDirectory)/src_helper.cpp$(ObjectSuffix): src/helper.cpp $(IntermediateDirectory)/src_helper.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/anton/1/cpp/2019/00armo/gamma/src/helper.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_helper.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_helper.cpp$(DependSuffix): src/helper.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_helper.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_helper.cpp$(DependSuffix) -MM src/helper.cpp

$(IntermediateDirectory)/src_helper.cpp$(PreprocessSuffix): src/helper.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_helper.cpp$(PreprocessSuffix) src/helper.cpp

$(IntermediateDirectory)/up_Display_ConsoleDisplay_src_display.cpp$(ObjectSuffix): ../Display/ConsoleDisplay/src/display.cpp $(IntermediateDirectory)/up_Display_ConsoleDisplay_src_display.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/anton/1/cpp/2019/00armo/Display/ConsoleDisplay/src/display.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_Display_ConsoleDisplay_src_display.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_Display_ConsoleDisplay_src_display.cpp$(DependSuffix): ../Display/ConsoleDisplay/src/display.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_Display_ConsoleDisplay_src_display.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_Display_ConsoleDisplay_src_display.cpp$(DependSuffix) -MM ../Display/ConsoleDisplay/src/display.cpp

$(IntermediateDirectory)/up_Display_ConsoleDisplay_src_display.cpp$(PreprocessSuffix): ../Display/ConsoleDisplay/src/display.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_Display_ConsoleDisplay_src_display.cpp$(PreprocessSuffix) ../Display/ConsoleDisplay/src/display.cpp

$(IntermediateDirectory)/src_LFSR.cpp$(ObjectSuffix): src/LFSR.cpp $(IntermediateDirectory)/src_LFSR.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/anton/1/cpp/2019/00armo/gamma/src/LFSR.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_LFSR.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_LFSR.cpp$(DependSuffix): src/LFSR.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_LFSR.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/src_LFSR.cpp$(DependSuffix) -MM src/LFSR.cpp

$(IntermediateDirectory)/src_LFSR.cpp$(PreprocessSuffix): src/LFSR.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_LFSR.cpp$(PreprocessSuffix) src/LFSR.cpp


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Release/


