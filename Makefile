NAME = ft_malcolm

CC = cc
CFLAGS = -Wall -Wextra -Werror -g3
RM = rm -f

OBJDIR = ./objects

OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

SRCS = main.c \

all: $(NAME)

$(NAME): $(OBJS)
	@$(CC) $(CFLAGS) -o $(NAME) $(OBJS)
	@$(MAKE) --no-print-directory show_msg


$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

show_msg:
	@echo "Compiled successfully!"
	@EXEC_LINE="=> execute ./$(NAME)"; \
	LEN=$$(echo "$$EXEC_LINE" | wc -c); \
	LEN=$$((LEN - 1)); \
	BORDER_LEN=$$((LEN + 2)); \
	TOP="┌$$(printf '─%.0s' $$(seq 1 $$BORDER_LEN))┐"; \
	MID="│ $$EXEC_LINE │"; \
	BOT="└$$(printf '─%.0s' $$(seq 1 $$BORDER_LEN))┘"; \
	echo "\033[35m$$TOP"; \
	echo "$$MID"; \
	echo "$$BOT\033[0m"

clean:
	@if [ -d "$(OBJDIR)" ]; then \
		echo "\033[33mCleaning object files...\033[0m"; \
		$(RM) -r $(OBJDIR); \
		echo "\033[32mClean completed successfully!\033[0m"; \
	else \
		echo "\033[33mNo objects to clean.\033[0m"; \
	fi

fclean: clean
	@if [ -f "$(NAME)" ]; then \
		echo "\033[33mRemoving $(NAME)...\033[0m"; \
		$(RM) $(NAME); \
		echo "\033[32mFull clean completed successfully!\033[0m"; \
	else \
		echo "\033[33mNo executable to clean.\033[0m"; \
	fi

re: fclean all

.PHONY: all show_msg libft clean re bonus