SRC		= src/luashell.cpp
BUILD	= build
LOGS	= $(BUILD)/logs
BIN		= $(BUILD)/bin
TESTS	= tests
TUNITS	= $(TESTS)/scripts
TLOGS	= $(TESTS)/logs
EXE		= $(BIN)/luashell
COPTS	= $(shell cat compile_flags.txt)
CPP		= g++

all: compile

compile: COPTS += -O3
compile: _compile

debug: COPTS += -g
debug: _compile

_compile: init
	@$(CPP) $(SRC) $(COPTS) -o $(EXE)

init:
	@mkdir -p $(BUILD) $(BIN)

clean:
	@git clean -d -f .
	@rm -r $(BUILD)

tests: init compile
	@mkdir -p $(LOGS)/$(TUNITS)
	@for f in $(TUNITS)/*; do $(EXE) $$f 2>&1 1>$(LOGS)/$$f.log && \
		diff $(TLOGS)/$$f.log $(LOGS)/$$f.log > $(LOGS)/$$f.difflog; \
		done;
	@find $(BUILD) -empty -delete
