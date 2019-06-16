#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

#include "pch.h"
#include "Peer.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma warning (disable : 4996)
// #pragma comment (lib, "Mswsock.lib")

using namespace myOpenCV;


const int alpha_slider_max = 100;
int alpha_slider;
double alpha;
Mat img;
Mat newImg;
vector<int> peers;




void on_trackbar(int, void*)
{
	clock_t time;
	char c = 1;
	time = clock();




	// WARNING
	if (&newImg != &img) {
		newImg = img.clone();
	}

	Rect rect;
	Mat imageTampon;
	for (int i =0; i < peers.size(); i++) {
		rect = Rect(0, i*(newImg.rows / (peers.size())), newImg.cols, (newImg.rows / (peers.size())));
		imageTampon = newImg(rect);

		// send to peers
		do_send(peers[i], &c, sizeof(char), 0);
		do_send_mat(peers[i], imageTampon);
		do_send(peers[i], &alpha_slider, sizeof(int), 0);
	}

	rect = Rect(0, peers.size()*(newImg.rows / peers.size()), newImg.cols, newImg.rows-peers.size()*(newImg.rows / peers.size()));
	Mat imageTamponReste = newImg(rect);

	saturationSettingThreaded(imageTamponReste, imageTamponReste, alpha_slider);

	newImg = Mat(0, 0, newImg.type());



	// receive from peers

	for (int i = 0; i < peers.size(); i++) {
		do_receive_mat(peers[i], imageTampon);
		newImg.push_back(imageTampon);
	}

	newImg.push_back(imageTamponReste);


	// END OF WARNING



	imshow("imageAfter", newImg);
	cout << "Total Time" << ((float)clock() - time) / CLOCKS_PER_SEC << endl;
}



int do_receive(int sockfd, void *buf, size_t len, int flags)
{
	size_t to_read = len;
	char* bufptr = (char*)buf;

	while (to_read > 0)
	{
		size_t rsz = recv(sockfd, bufptr, to_read, flags);
		if (rsz <= 0) {
			return rsz;  /* Error or other end closed connection */
		}

		to_read -= rsz;  /* Read less next time */
		bufptr += rsz;  /* Next buffer position to read into */
	}

	return len;
}

int do_send(int sockfd, void* msg, size_t len, int flags)
{
	size_t to_write = len;
	char* bufptr = (char*)msg;

	while (to_write > 0)
	{
		size_t rsz = send(sockfd, bufptr, to_write, flags);
		if (rsz <= 0) {
			return rsz;  /* Error or other end closed connection */
		}

		to_write -= rsz;  /* Write less next time */
		bufptr += rsz;  /* Next buffer position to write into */
	}

	return len;
}

int do_receive_image(int sockfd)
{
	char buff[10240];
	int size, recv = 0;
	FILE *image;
	
	image = fopen("tmp.jpg", "wb");
	if (image == NULL) {
		printf("Error has occurred. Image file could not be opened\n");
		return -1;
	}

	do_receive(sockfd, &size, sizeof(int), 0);

	while (recv < size) {

		recv += do_receive(sockfd, &buff, sizeof(buff), 0);
		fwrite(&buff, sizeof(char), sizeof(buff), image);
	}
	
	fclose(image);
	return 1;
}

int do_send_image(int sockfd, const char* filename)
{
	FILE *image;
	int size, read;
	char buff[10240];

	image = fopen(filename, "rb");
	if (image == NULL) {
		printf("Error Opening Image File");
		return -1;
	}

	fseek(image, 0, SEEK_END);
	size = ftell(image);
	fseek(image, 0, SEEK_SET);
	do_send(sockfd, &size, sizeof(int), 0);

	while (!feof(image)) {

		read = fread(&buff, sizeof(char), sizeof(buff), image);
		do_send(sockfd, &buff, sizeof(buff), 0);

		//Zero out our send buffer
		memset(buff, '\0', sizeof(buff));
	}

	fclose(image);
	return 1;
}

int do_send_mat(int sockfd, Mat & frame)
{
	int height, width;

	height = frame.rows;
	width = frame.cols;

	frame = (frame.reshape(0, 1)); // to make it continuous

	int size = frame.total() * frame.elemSize();

	do_send(sockfd, &height, sizeof(int), 0);
	do_send(sockfd, &width, sizeof(int), 0);

	return do_send(sockfd, frame.data, size, 0);
}

int do_receive_mat(int sockfd, Mat & frame)
{
	int height, width, size, ptr = 0;
	uchar* data;

	// receive dimension
	do_receive(sockfd, &height, sizeof(int), 0);
	do_receive(sockfd, &width, sizeof(int), 0);

	frame = Mat::zeros(height, width, CV_8UC3);
	size = frame.total() * frame.elemSize();
	data = (uchar*)malloc(size * sizeof(uchar));

	//Receive data
	do_receive(sockfd, data, size, 0);

	// Assign pixel value to img
	for (int i = 0; i < frame.rows; i++) {
		for (int j = 0; j < frame.cols; j++) {
			frame.at<cv::Vec3b>(i, j) = cv::Vec3b(data[ptr + 0], data[ptr + 1], data[ptr + 2]);
			ptr = ptr + 3;
		}
	}

	return 0;
}

void zero_info(struct addrinfo* hints, struct addrinfo* result)
{
	ZeroMemory(hints, sizeof(*hints));
	hints->ai_family = AF_INET;
	hints->ai_socktype = SOCK_STREAM;
	hints->ai_protocol = IPPROTO_TCP;
	//hints->ai_flags = AI_PASSIVE;

	freeaddrinfo(result);
	result = NULL;
}





int __cdecl main(void)
{
	WSADATA wsa_data;
	int i_result;

	SOCKET listen_socket = INVALID_SOCKET;
	SOCKET peers_socket = INVALID_SOCKET;

	struct addrinfo *result = NULL, *ptr = NULL;
	struct addrinfo hints;

	int recvbuflen = DEFAULT_BUFLEN;

	fd_set sock_read;
	int rec_val;
	int max_sd;
	char port[33];

	int nb_already_connected = -1;



	// Initialize Winsock
	i_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (i_result != 0) {
		printf("WSAStartup failed with error: %d\n", i_result);
		return 1;
	}







	zero_info(&hints, result);

	// Resolve the server address and port
	i_result = getaddrinfo(DEFAULT_ADDR, DEFAULT_PORT, &hints, &result);
	if (i_result != 0) {
		printf("getaddrinfo failed with error: %d\n", i_result);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to default address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to some peer
		peers_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (peers_socket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		i_result = connect(peers_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (i_result == SOCKET_ERROR) {
			closesocket(peers_socket);
			peers_socket = INVALID_SOCKET;
			continue;
		}

		peers.push_back(peers_socket);
		break;
	}

	// There was already a peer ready
	if (peers.size() > 0)
	{
		//how many are already connected ?
		do_receive(peers_socket, &nb_already_connected, sizeof(int), 0);

		if (nb_already_connected > 0) {
			printf("There is already %d peers connected !\n", nb_already_connected);
		}
		else {
			printf("I'm the second one !\n");
		}
		
		// connect to everyone already here
		for(int i = 1; i < nb_already_connected + 1; i++)
		{
			printf("Try to connect to port %d\n", atoi(DEFAULT_PORT) + i);

			zero_info(&hints, result);

			memset(port, 0, sizeof(port));
			itoa(atoi(DEFAULT_PORT) + i, port, 10);
			i_result = getaddrinfo(DEFAULT_ADDR, port, &hints, &result);
			if (i_result != 0) {
				printf("getaddrinfo failed with error: %d\n", i_result);
				WSACleanup();
				return 1;
			}

			// Attempt to connect to shifted address until one succeeds
			for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

				// Create a SOCKET for connecting to some peer
				peers_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
				if (peers_socket == INVALID_SOCKET) {
					printf("socket failed with error: %ld\n", WSAGetLastError());
					WSACleanup();
					return 1;
				}

				// Connect to peer.
				i_result = connect(peers_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
				if (i_result == SOCKET_ERROR) {
					closesocket(peers_socket);
					peers_socket = INVALID_SOCKET;
					continue;
				}

				peers.push_back(peers_socket);
				break;
			}

			int useless = 0;
			do_receive(peers_socket, &useless, sizeof(int), 0);
		}

		printf("I'm connected to everyone !\n");
	}
	else
	{
		printf("I'm the first peer !\n");
	}








	img = imread("C:\\Users\\hugor\\Downloads\\ironman.jpg");
	setUseOptimized(true);
	
	if (!img.empty())
	{
		namedWindow("image", WINDOW_NORMAL);
		namedWindow("imageAfter", WINDOW_NORMAL);
	
		alpha_slider = 0;
	
		createTrackbar("test", "imageAfter", &alpha_slider, alpha_slider_max, on_trackbar);
		imshow("image", img);
		on_trackbar(alpha_slider, 0);
	
		waitKey(0);
	}










	zero_info(&hints, result);

	memset(port, 0, sizeof(port));
	itoa(atoi(DEFAULT_PORT) + nb_already_connected + 1, port, 10);

	printf("My port is %s\n", port);

	i_result = getaddrinfo(DEFAULT_ADDR, port, &hints, &result);
	if (i_result != 0) {
		printf("getaddrinfo failed with error: %d\n", i_result);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for listening new peers
	listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_socket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	i_result = ::bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);
	if (i_result == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	// Listen
	i_result = listen(listen_socket, SOMAXCONN);
	if (i_result == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	max_sd = listen_socket;

	freeaddrinfo(result);

	while (true)
	{
		FD_ZERO(&sock_read);
		FD_SET(listen_socket, &sock_read);
		for (const auto& fd : peers) {
			FD_SET(fd, &sock_read);
		}

		printf("\nWaiting for peers...\n");

		rec_val = select(max_sd + 1, &sock_read, NULL, NULL, NULL);

		if (rec_val <= 0) {
			return EXIT_FAILURE;
		}

		for (int fd = 0; fd <= max_sd && rec_val > 0; fd++)
		{
			if (!FD_ISSET(fd, &sock_read)) { continue; }

			if (fd == listen_socket) // ADDING NEW PEERS
			{
				peers_socket = accept(listen_socket, NULL, NULL);
				if (peers_socket == INVALID_SOCKET) {
					printf("accept failed with error: %d\n", WSAGetLastError());
					closesocket(listen_socket);
					WSACleanup();
					return 1;
				}

				printf("New peer connected !\n");

				int size = peers.size();
				do_send(peers_socket, &size, sizeof(int), 0);

				peers.push_back(peers_socket);

				if (peers_socket > max_sd) {
					max_sd = peers_socket;
				}
			}
			else // RECEIVING / SENDING for 'fd'
			{
				printf("Peers %d want to tell me something !\n", fd);

				char mess = 0;

				if (do_receive(fd, &mess, sizeof(char), 0) <= 0)
				{
					i_result = shutdown(fd, SD_SEND);
					if (i_result == SOCKET_ERROR) {
						printf("shutdown failed with error: %d\n", WSAGetLastError());
						closesocket(peers_socket);
						WSACleanup();
						return 1;
					}
					closesocket(fd);

					printf("Peers %d successfully deconnected !\n", fd);

					peers.erase(std::remove(peers.begin(), peers.end(), fd), peers.end());
				}

				// receive image from peers
				Mat frame;
				do_receive_mat(fd, frame);

				// process image

				// re-send image to askers
				do_send_mat(fd, frame);
			}

			rec_val--;
		}
	}

	// No longer need server socket
	closesocket(listen_socket);
	WSACleanup();

	for(int i = 0; i <= max_sd; ++i)
	{
		if (FD_ISSET(i, &sock_read)) {
			closesocket(i);
		}	
	}

	return EXIT_SUCCESS;
}