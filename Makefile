NAME      := webserv
CXX       := c++
CXXFLAGS  := -Wall -Wextra -Werror -std=c++98
CPPFLAGS  := -I includes -MMD -MP

BUILD_DIR := build

include source.mk

OBJ := $(patsubst srcs/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEP := $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

$(BUILD_DIR)/%.o: srcs/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

debug: CXXFLAGS += -g3
debug: re

sanitize: CXXFLAGS += -g3 -fsanitize=address -fno-omit-frame-pointer
sanitize: re

test-m1: $(NAME)
	@echo "Run server in another terminal: ./$(NAME) conf/default.conf"
	bash tests/milestone1/run_all.sh

test-config: CXXFLAGS += -g
test-config:
	@mkdir -p $(BUILD_DIR)/tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c tests/test_config_parser.cpp -o $(BUILD_DIR)/tests/test_config_parser.o
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c srcs/config/Config.cpp -o $(BUILD_DIR)/config/Config.o
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c srcs/config/ConfigParser.cpp -o $(BUILD_DIR)/config/ConfigParser.o
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c srcs/config/Location.cpp -o $(BUILD_DIR)/config/Location.o
	$(CXX) $(CXXFLAGS) $(BUILD_DIR)/tests/test_config_parser.o $(BUILD_DIR)/config/Config.o $(BUILD_DIR)/config/ConfigParser.o $(BUILD_DIR)/config/Location.o -o test_config_parser
	@echo "\nRunning config parser test..."
	./test_config_parser conf/default.conf

print-%:
	@echo '$*=$($*)'

-include $(DEP)

.PHONY: all clean fclean re debug sanitize test-m1 test-config print-%
