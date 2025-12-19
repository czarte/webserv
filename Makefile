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

print-%:
	@echo '$*=$($*)'

-include $(DEP)

.PHONY: all clean fclean re debug sanitize test-m1 print-%
