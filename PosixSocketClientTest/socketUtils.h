#ifndef _SOCKETUTILS_H_
#define _SOCKETUTILS_H_

void error(std::string msg)
{
    perror(msg.c_str());
    exit(1);
}

class SocketUtils
{
public:
	SocketUtils() {};
	~SocketUtils() {};

	static int establishSocket(int portno)
	{
	    int sockfd, newSockfd, clilen;
	    struct sockaddr_in serv_addr, cli_addr;

	    sockfd = socket(AF_INET, SOCK_STREAM, 0); //AF_UNIX
	    if (sockfd < 0)
	        error("ERROR opening socket");
	    std::cout << "connected to socket " << sockfd << std::endl;

	    bzero((char *) &serv_addr, sizeof(serv_addr));
	    serv_addr.sin_family = AF_INET;
	    serv_addr.sin_addr.s_addr = INADDR_ANY;
	    serv_addr.sin_port = htons(portno);
	    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	    	error("ERROR on binding");
	    std::cout << "binding to port " << portno << std::endl;

	    listen(sockfd,5);
	    std::cout << "listened to socket " << sockfd << std::endl;

	    clilen = sizeof(cli_addr);

	    newSockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);
		if (newSockfd < 0)
			error("ERROR on accept");
	    std::cout << "accepted: new socket " << newSockfd << std::endl;

		close(sockfd);

		return newSockfd;
	}

private:
};


#endif //_SOCKETUTILS_H_
