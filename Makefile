#==================
# compile flags
#==================

PARENT = ..
LIB_SRC_PATH = .
INCLUDE_PATH = include
LIB_PATH = $(EXTERN_LIB_PATH)
SRC_PATH = src
BUILD_PATH = build
BIN_PATH = bin
BIN_STEMS = main_conway main_maze
BINARIES = $(patsubst %, $(BIN_PATH)/%, $(BIN_STEMS))

INCLUDE_PATHS = $(INCLUDE_PATH) $(EXTERN_INCLUDE_PATH)
INCLUDE_PATH_FLAGS = $(patsubst %, -I%, $(INCLUDE_PATHS))

LIB_PATHS = $(LIB_PATH)
LIB_PATH_FLAGS = $(patsubst %, -L%, $(LIB_PATHS))

LIB_STEMS = glut GLEW GL png
LIBS = $(patsubst %, $(LIB_PATH)/lib%.a, $(LIB_STEMS))
LIB_FLAGS = $(patsubst %, -l%, $(LIB_STEMS))

CXX = g++
DEBUG = -g
CXXFLAGS = -Wall $(DEBUG) $(INCLUDE_PATH_FLAGS) -std=c++0x -DGLM_ENABLE_EXPERIMENTAL=1
LDFLAGS = -Wall $(DEBUG) $(LIB_PATH_FLAGS) $(LIB_FLAGS)

SCRIPT_PATH = scripts

#==================
# all
#==================

.DEFAULT_GOAL : all
all : $(BINARIES)

#==================
# bin_only
#==================

.PHONY : bin_only
bin_only : $(BINARIES)

#==================
# for_travis
#==================

.PHONY : for_travis
for_travis : CXXFLAGS += -DNO_GLM_CONSTANTS

for_travis : bin_only
	@echo TARGET=$@ CXXFLAGS=${CXXFLAGS}

#==================
# objects
#==================

$(BUILD_PATH)/%.o : $(SRC_PATH)/%.cpp
	mkdir -p $(BUILD_PATH)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(BUILD_PATH)/%.o : $(SRC_PATH)/%.c
	mkdir -p $(BUILD_PATH)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

.PHONY : clean_objects
clean_objects :
	-rm $(CONWAY_OBJECTS) $(MAZE_OBJECTS)

#==================
# binaries
#==================

SHARED_CPP_STEMS = BBoxObject \
                   Buffer \
                   Camera \
                   File3ds \
                   FilePng \
                   FrameBuffer \
                   IdentObject \
                   KeyframeMgr \
                   Light \
                   Modifiers \
                   Material \
                   Mesh \
                   NamedObject \
                   Octree \
                   PrimitiveFactory \
                   Program \
                   Scene \
                   Shader \
                   ShaderContext \
                   shader_utils \
                   Texture \
                   Util \
                   VarAttribute \
                   VarUniform \
                   TransformObject
CONWAY_CPP_STEMS = $(SHARED_CPP_STEMS) main_conway
CONWAY_OBJECTS   = $(patsubst %, $(BUILD_PATH)/%.o, $(CONWAY_CPP_STEMS))
MAZE_CPP_STEMS = $(SHARED_CPP_STEMS) main_maze
MAZE_OBJECTS   = $(patsubst %, $(BUILD_PATH)/%.o, $(MAZE_CPP_STEMS))
LINT_FILES = $(patsubst %, $(BUILD_PATH)/%.lint, $(SHARED_CPP_STEMS))

$(BIN_PATH)/main_conway : $(CONWAY_OBJECTS)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BIN_PATH)/main_maze : $(MAZE_OBJECTS)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)

.PHONY : clean_binaries
clean_binaries :
	-rm $(BINARIES)

#==================
# test
#==================

TEST_PATH = test
TEST_SH = $(SCRIPT_PATH)/test.sh
TEST_FILE_STEMS = main_conway main_maze
TEST_FILES = $(patsubst %, $(BUILD_PATH)/%.test, $(TEST_FILE_STEMS))
TEST_PASS_FILES = $(patsubst %, %.pass, $(TEST_FILES))
TEST_FAIL_FILES = $(patsubst %, %.fail, $(TEST_FILES))

$(BUILD_PATH)/main_conway.test.pass : $(BIN_PATH)/main_conway $(TEST_PATH)/main_conway.test
	-$(TEST_SH) $(BIN_PATH)/main_conway arg $(TEST_PATH)/main_conway.test $(TEST_PATH)/main_conway.gold \
			$(BUILD_PATH)/main_conway.test

$(BUILD_PATH)/main_maze.test.pass : $(BIN_PATH)/main_maze $(TEST_PATH)/main_maze.test
	-$(TEST_SH) $(BIN_PATH)/main_maze arg $(TEST_PATH)/main_maze.test $(TEST_PATH)/main_maze.gold \
			$(BUILD_PATH)/main_maze.test

.PHONY : test
test : $(TEST_PASS_FILES)

.PHONY : clean_tests
clean_tests :
	-rm $(TEST_PASS_FILES) $(TEST_FAIL_FILES)

#==================
# lint
#==================

LINT_PASS_FILES = $(patsubst %, %.pass, $(LINT_FILES))
LINT_FAIL_FILES = $(patsubst %, %.fail, $(LINT_FILES))
LINT_SH = $(SCRIPT_PATH)/lint.sh

$(BUILD_PATH)/%.lint.pass : $(SRC_PATH)/%.c*
	mkdir -p $(BUILD_PATH)
	-$(LINT_SH) $< $(BUILD_PATH)/$*.lint $(INCLUDE_PATH_FLAGS)

.PHONY : lint
lint : $(LINT_PASS_FILES)

.PHONY : clean_lint
clean_lint :
	-rm $(LINT_PASS_FILES) $(LINT_FAIL_FILES)

#==================
# docs
#==================

DOC_PATH = docs
DOC_CONFIG_FILE = dexvt-gpgpu-ping-pong-technique.config
DOC_CONFIG_PATCH_FILE = $(DOC_CONFIG_FILE).patch
DOC_TOOL = doxygen

.PHONY : docs
docs :
	mkdir -p $(BUILD_PATH)
	doxygen -g $(BUILD_PATH)/$(DOC_CONFIG_FILE)
	patch $(BUILD_PATH)/$(DOC_CONFIG_FILE) < $(DOC_PATH)/$(DOC_CONFIG_PATCH_FILE)
	cd $(BUILD_PATH); $(DOC_TOOL) $(DOC_CONFIG_FILE)

.PHONY : clean_docs
clean_docs :
	rm -rf $(BUILD_PATH)/html
	rm -rf $(BUILD_PATH)/$(DOC_CONFIG_FILE)

#==================
# clean
#==================

.PHONY : clean
clean : clean_binaries clean_objects clean_tests clean_lint #clean_docs
	-rmdir $(BUILD_PATH) $(BIN_PATH)
