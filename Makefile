NAME		= codexion

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -pthread -g3
INCLUDES	= -I inc

SRC_DIR		= src
OBJ_DIR		= obj
INC_DIR		= inc

SRCS		= $(SRC_DIR)/coder.c \
			  $(SRC_DIR)/dongle.c \
			  $(SRC_DIR)/dongle_utils.c \
			  $(SRC_DIR)/log.c \
			  $(SRC_DIR)/log_burnout.c \
			  $(SRC_DIR)/main.c \
			  $(SRC_DIR)/monitor.c \
			  $(SRC_DIR)/parse_args.c \
			  $(SRC_DIR)/pqueue.c \
			  $(SRC_DIR)/pqueue_utils.c \
			  $(SRC_DIR)/sim.c \
			  $(SRC_DIR)/sim_run.c \
			  $(SRC_DIR)/sim_time.c

OBJS		= $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

HEADERS		= $(INC_DIR)/codexion.h

RM			= rm -f
RMDIR		= rm -rf

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	$(RMDIR) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
