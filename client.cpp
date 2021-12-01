#include <iostream>
#include <bits/stdc++.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#define chunk_size 5120

using namespace std;

struct file_struct
{
	string owner_name;
	string ip;
	int port;
	vector<int> v_chunk;
	int chunk_count;
	int socket = -1;
	string f_path;
	string d_path;
	string hash;
};

unordered_map<string, string> map_file;
int tracker;

void *send_file(void *s)
{
	int socket = *(int *)s;
	vector<string> ip_c;
	char msg[chunk_size];
	bzero(msg, chunk_size);
	string str;
	recv(socket, msg, sizeof(msg), 0);
	string command(msg);
	stringstream ss(command);
	while (ss >> str)
		ip_c.push_back(str);

	if (ip_c[0] == "get_chunk_details")
	{
		char msg[chunk_size];
		strcpy(msg, map_file[ip_c[1]].c_str());
		send(socket, msg, strlen(msg), 0);
	}
	string t = ip_c[0];
	if (t == "download")
	{
		char buffer[chunk_size];
		FILE *file_pointer = fopen(ip_c[1].c_str(), "rb+");
		char ack[512];
		strcpy(ack, "get chunk");
		send(socket, ack, strlen(ack), 0);
		int chunk_number;
		recv(socket, &chunk_number, sizeof(int), 0);
		cout << "Chunk Number :- " << chunk_number << endl;
		int start = chunk_number * chunk_size;
		fseek(file_pointer, start, SEEK_SET);
		//bzero(buffer, chunk_size);
		int leng = fread(buffer, sizeof(char), sizeof(buffer), file_pointer);
		send(socket, buffer, leng, 0);
		cout << "Chunk successfully sent and size is :- " << leng << endl;
		fclose(file_pointer);
	}
}

void *as_server(void *p)
{
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (server_socket == -1)
	{
		cout << "Error occured while opening socket." << endl;
	}
	int *tmp_var = (int *)p;
	int port = *tmp_var;
	struct sockaddr_in server_add;
	bzero((char *)&server_add, sizeof(server_add));
	server_add.sin_port = htons(port);
	server_add.sin_addr.s_addr = INADDR_ANY;
	server_add.sin_family = AF_INET;
	int x = bind(server_socket, (struct sockaddr *)&server_add, sizeof(server_add));
	int i = 0;
	if (x < 0)
		cout << "Binding is not successful." << endl;

	listen(server_socket, 10);
	int all_socket[1000];
	while (true)
	{
		pthread_t handel_req;
		struct sockaddr_in address_client;
		socklen_t len = sizeof(address_client);
		all_socket[i] = accept(server_socket, (struct sockaddr *)&address_client, &len);

		pthread_create(&handel_req, NULL, send_file, &all_socket[i]);
		if (all_socket[i] < 0)
		{
			cout << "Error occured while accepting Request!" << endl;
		}
		i = (i + 1) % 1000;
	}
	close(server_socket);
}

int make_connection(int port, string ip)
{
	struct sockaddr_in server_add;
	int t_soc = socket(AF_INET, SOCK_STREAM, 0);
	char ip_add[64];
	strcpy(ip_add, ip.c_str());
	if (t_soc == -1)
	{
		cout << "Error while opening current socket." << endl;
		return -1;
	}
	struct hostent *server = gethostbyname(ip_add);
	if (server == NULL)
	{
		close(t_soc);
		cout << "Host doesn't exist right now." << endl;
		return -1;
	}

	bzero((char *)&server_add, sizeof(server_add));
	server_add.sin_family = AF_INET;
	server_add.sin_port = htons(port);
	bcopy((char *)server->h_addr, (char *)&server_add.sin_addr.s_addr, server->h_length);
	int x = connect(t_soc, (struct sockaddr *)&server_add, sizeof(server_add));
	if (x < 0)
	{
		close(t_soc);
		cout << "Error occured while connecting." << endl;
		return -1;
	}
	return t_soc;
}

void upload_file(string ip)
{
	string tmp = ip;
	stringstream s(tmp);
	string file_path;
	s >> file_path >> file_path;
	string final_path = file_path;
	FILE *file_pointer = fopen(final_path.c_str(), "rb+");
	if (file_pointer == NULL)
	{
		cout << "Invalid file path!" << endl;
		return;
	}

	fseek(file_pointer, 0, SEEK_END);
	long long size = ftell(file_pointer);
	cout << "FILE size:- " << size << endl;
	fclose(file_pointer);
	char ch[64];
	sprintf(ch, "%lld", size);
	char msg[chunk_size];
	strcpy(msg, ip.c_str());
	strcat(msg, " ");
	strcat(msg, final_path.c_str());
	strcat(msg, "hjkbu8pshu898s8ju6shvh6tdhabc876mjugc4gc8cl9i0hjktuk876pkjn5f6fg7h8cvgtyh8734gtikjnpasf67");
	strcat(msg, " ");
	strcat(msg, ch);
	send(tracker, msg, strlen(msg), 0);
	bzero(msg, chunk_size);
	recv(tracker, msg, sizeof(msg), 0);
	long long no_chunks;
	string str;
	if (!strcmp(msg, "File successfully uploaded."))
	{
		no_chunks = size / chunk_size;
		int i = 0;
		string tmp = to_string(i++);
		str = tmp;
		while (i <= no_chunks)
		{
			i++;
			tmp = to_string(i);
			str = str + " ";
			str = str + tmp;
		}
		map_file[final_path] = str;
	}
	cout << msg << endl;
}

bool cmp(struct file_struct *ow1, struct file_struct *ow2)
{
	int tmp1 = ow1->v_chunk.size();
	int tmp2 = ow2->v_chunk.size();
	if (tmp1 > tmp2)
		return false;
	else
		return true;
}

void *handle_thread(void *tmp_p)
{
	char msg[256];
	struct file_struct *f_struct = (struct file_struct *)tmp_p;
	char c_buff[chunk_size];
	int socket = make_connection(f_struct->port, f_struct->ip);
	if (socket == -1)
	{
		return NULL;
	}
	strcpy(msg, "get_chunk_details ");
	strcat(msg, f_struct->f_path.c_str());
	send(socket, msg, strlen(msg), 0);
	bzero(c_buff, chunk_size);
	recv(socket, c_buff, sizeof(c_buff), 0);
	int c;
	string tmp = c_buff;
	stringstream s(tmp);
	while (s >> c)
	{
		f_struct->v_chunk.push_back(c);
	}
}

void *download(void *tmp_p)
{
	struct file_struct *f_struct = (struct file_struct *)tmp_p;
	FILE *file_pointer = fopen(f_struct->d_path.c_str(), "rb+");

	int start = f_struct->chunk_count * chunk_size;
	if (file_pointer == NULL)
	{
		cout << " Found Invalid destination path!" << endl;
	}
	fseek(file_pointer, start, SEEK_SET);
	char msg[512];
	f_struct->socket = make_connection(f_struct->port, f_struct->ip);
	if (f_struct->socket == -1)
	{
		cout << "Error in connection" << endl;
		return NULL;
	}
	strcpy(msg, "download ");
	strcat(msg, f_struct->f_path.c_str());
	send(f_struct->socket, msg, sizeof(msg), 0);
	char ack[512];
	recv(f_struct->socket, ack, sizeof(ack), 0);
	send(f_struct->socket, &f_struct->chunk_count, sizeof(int), 0);

	char buffer[chunk_size];
	int leng = recv(f_struct->socket, buffer, sizeof(buffer), 0);
	fwrite(buffer, sizeof(char), leng, file_pointer);
	int cco = f_struct->chunk_count;
	cout << cco << " successfully downloaded from " << f_struct->owner_name << endl;
	fclose(file_pointer);
	sleep(0.1);
}

void send_cmd(int n, string command)
{
	char msg[n];
	strcpy(msg, command.c_str());
	send(tracker, msg, strlen(msg), 0);
	bzero(msg, n);
	recv(tracker, msg, sizeof(msg), 0);
	cout << msg << endl;
}

void download_file(string command)
{
	int file_size;
	char msg[chunk_size];
	vector<string> v;
	vector<struct file_struct *> owners;
	strcpy(msg, command.c_str());
	send(tracker, msg, strlen(msg), 0);
	bzero(msg, chunk_size);
	recv(tracker, msg, sizeof(msg), 0);
	string hash;

	if (strstr(msg, "/"))
	{
		char *ch_ptr;
		file_size = atoi(strtok(msg, "/"));
		hash = strtok(NULL, "/");
		bzero(msg, chunk_size);
		recv(tracker, msg, sizeof(msg), 0);
		ch_ptr = strtok(msg, "\n");
		string tmp;
		while (ch_ptr)
		{
			tmp = ch_ptr;
			v.push_back(tmp);
			ch_ptr = strtok(NULL, "\n");
		}
	}
	else
	{
		cout << msg << endl;
		return;
	}

	char *file_name = strtok((char *)command.c_str(), " ");
	file_name = strtok(NULL, " ");
	file_name = strtok(NULL, " ");
	file_name = strtok(NULL, " ");
	cout << "Downloading file....." << endl;
	for (auto i : v)
	{
		struct file_struct *t = new struct file_struct;
		stringstream s(i);
		s >> t->owner_name;
		s >> t->port;
		s >> t->ip;
		s >> t->f_path;
		t->d_path = file_name;
		t->hash = hash;
		owners.push_back(t);
	}
	int len = owners.size();
	for (int i = 0; i < len; i++)
	{
		pthread_t th;
		pthread_create(&th, NULL, handle_thread, owners[i]);
		pthread_join(th, NULL);
	}

	sort(owners.begin(), owners.end(), cmp);
	for (int i = 0; i < len; i++)
	{
		cout << "owner name " << owners[i]->owner_name << endl;
		for (int j = 0; j < owners[i]->v_chunk.size(); j++)
		{
			cout << owners[i]->v_chunk[j] << " ";
		}
		cout << endl;
	}
	FILE *file_pointer = fopen(file_name, "wb");
	int t = file_size;
	char ch[2];
	strcpy(ch, "0");
	int no_chunks = file_size / chunk_size;
	while (t > 0)
	{
		fwrite(ch, sizeof(char), 1, file_pointer);
		t--;
	}
	fclose(file_pointer);

	vector<bool> is_downloaded(no_chunks + 1, 0);
	vector<int> ind(owners.size(), 0);
	int flag = 1;
	while (flag)
	{
		flag = 0;
		for (int i = 0; i < owners.size(); i++)
		{
			while (ind[i] < owners[i]->v_chunk.size() && is_downloaded[owners[i]->v_chunk[ind[i]]])
			{
				ind[i]++;
			}
			if (ind[i] < owners[i]->v_chunk.size())
			{
				flag = 1;
				is_downloaded[owners[i]->v_chunk[ind[i]]] = 1;
				cout << "Chunk download start--" << owners[i]->v_chunk[ind[i]] << endl;
				owners[i]->chunk_count = owners[i]->v_chunk[ind[i]];
				cout << "Socket in download" << owners[i]->socket << endl;
				pthread_t t;
				pthread_create(&t, NULL, download, owners[i]);
				pthread_join(t, NULL);
				cout << "Chunk download done--" << owners[i]->v_chunk[ind[i]] << endl;
				cout << "Socket in download" << owners[i]->socket << endl;
			}
		}
	}
}

int main(int argc, char const *argv[])
{
	char *tracker_path;
	char *tmp;
	if (argc != 3)
	{
		cout << "Correct way to Run this is: ./peer <IP>:<PORT> <Path of tracker_info.txt>" << endl;
		return 0;
	}
	tracker_path = (char *)malloc(128 * sizeof(char));
	pthread_t t;
	strcpy(tracker_path, argv[2]);
	tmp = strtok((char *)argv[1], ":");
	string ip(tmp);
	int port = atoi(strtok(NULL, ":"));
	// cout<<port;
	pthread_create(&t, NULL, as_server, (void *)&port);
	int tracker_port;
	string tracker_ip;
	tracker_ip = "127.0.0.1";
	tracker_port = 8001;
	tracker = make_connection(tracker_port, tracker_ip);
	if (tracker > 0)
	{
		cout << "successfully connected with tracker!" << endl;
	}
	string command;
	while (true)
	{
		vector<string> ip_c;
		cout << "Enter command:";
		getline(cin, command);
		if (command.size() == 0)
			continue;

		stringstream s(command);
		string temp;
		while (s >> temp)
			ip_c.push_back(temp);

		string c = ip_c[0];

		if (c == "create_user")
		{
			char t[16];
			sprintf(t, "%d", port);
			char msg[512];
			strcpy(msg, command.c_str());
			strcat(msg, " ");
			strcat(msg, ip.c_str());
			strcat(msg, " ");
			strcat(msg, t);
			send(tracker, msg, strlen(msg), 0);
			bzero(msg, 512);
			recv(tracker, msg, sizeof(msg), 0);
			cout << msg << endl;
		}
		else if (c == "login")
		{
			send_cmd(512, command);
		}
		else if (c == "create_group")
		{
			send_cmd(512, command);
		}
		else if (c == "join_group")
		{
			send_cmd(512, command);
		}
		else if (c == "leave_group")
		{
			send_cmd(512, command);
		}
		else if (c == "list_requests")
		{
			send_cmd(2048, command);
		}
		else if (c == "accept_request")
		{
			send_cmd(512, command);
		}
		else if (c == "list_groups")
		{
			send_cmd(1024, command);
		}
		else if (c == "list_files")
		{
			send_cmd(51200, command);
		}
		else if (c == "upload_file")
		{
			//upload_file <filepath> <groupId> <hash> <size>
			upload_file(command);
		}
		else if (c == "download_file")
		{
			download_file(command);
		}
		else if (c == "logout")
		{
			send_cmd(512, command);
		}
		else if (c == "show_downloads")
		{
			send_cmd(512, command);
		}
		else if (c == "stop_share")
		{
			send_cmd(512, command);
		}
		else
		{
			send_cmd(512, command);
		}
	}

	pthread_exit(NULL);
	return 0;
}