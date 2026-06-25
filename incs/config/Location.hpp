#ifndef LOCATION_HPP
# define LOCATION_HPP

# include <iostream>
# include <string>
# include <vector>


class Location
{
private:
	std::string					_path; //for /images or just /
	std::string					_root; //for physical directory on our machine
	std::vector<std::string>	_index; //for default files like index.html
	bool						_autoindex; //directory listing on/off
	std::vector<std::string>	_methods; //GET, POST, DELETE
	std::string					_cgiExtension; 
	std::string					_cgiPath;

public:
	Location();
	Location(const Location& other);
	Location& operator=(const Location& other);
	~Location();

	//setters
	void	setPath(const std::string& path);
	void	addIndex(const std::string& indexPage);
	void	setRoot(const std::string& root);
	void	setAutoindex(bool autoIndex);
	void	addMethod(const std::string& method);
	void	setCgiPath(const std:: string& cgiPath);
	void	setCgiExtension(const std::string& cgiExt);
	//getters

};

#endif