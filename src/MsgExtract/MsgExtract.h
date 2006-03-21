#pragma once

class Connection;

class MsgExtract
{
public:
	MsgExtract(int argc, const char **argv);
	~MsgExtract(void);
	
	Connection	*connection;
	void genAll(void);
	void concat(const char *source, char **target);
	
	const char *database;
};
