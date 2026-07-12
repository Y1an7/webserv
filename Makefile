# NAME		= webserv
# CXX			= c++
# CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -I incs/

# SRC_DIR		= srcs/
# OBJ_DIR		= objs/

# SRCS		=	main.cpp \
# 				config/ConfigParser.cpp \
# 				config/Location.cpp \
# 				config/ServerConfig.cpp \
# 				core/Utils.cpp \
# 				network/Client.cpp \
# 				network/Server.cpp \
# 				network/ServerSocket.cpp \
# 				http/HttpRequest.cpp \
# 				http/HttpParser.cpp \
# 				http/HttpResponse.cpp \
# 				cgi/CgiHandler.cpp

# OBJS		= $(addprefix $(OBJ_DIR), $(SRCS:.cpp=.o))

# all: $(NAME)

# $(NAME): $(OBJS)
# 				@echo "Linking $(NAME)..."
# 				@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
# 				@echo "$(NAME) compiled successfully!"

# $(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
# 				@mkdir -p $(dir $@)
# 				@$(CXX) $(CXXFLAGS) -c $< -o $@

# clean:
# 				@echo "Cleaning object files..."
# 				@rm -rf $(OBJ_DIR)

# fclean: clean
# 				@echo "Cleaning $(NAME)..."
# 				@rm -f $(NAME)

# re: fclean all

# .PHONY: all clean fclean re



NAME        = webserv_test
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -I. -Iincs/config -Iincs/network

# NO Server.cpp here
SRCS        = srcs/main.cpp \
              srcs/config/ConfigParser.cpp \
              srcs/config/ServerConfig.cpp \
              srcs/config/Location.cpp \
              srcs/network/ServerSocket.cpp

OBJS        = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all