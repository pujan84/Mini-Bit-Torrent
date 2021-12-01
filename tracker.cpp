#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sstream>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <netdb.h>

using namespace std;

struct file_detail
{
	vector<string> file_path;
	vector<string> grp;
	vector<string> file_owners;
	string total_size;
};

struct user_detail
{
	string user_ip;
	string pwd;
	vector<string> user_grp;
	vector<string> user_file;
	string user_port;
};

struct group_detail
{
	string grp_owner;
	vector<string> grp_members;
	vector<string> join_reqs;
	string group_id;
};

unordered_map<string, struct user_detail *> users;
unordered_map<string, string> map_file;
unordered_map<string, int> login_status;
unordered_map<string, struct file_detail *> files;
unordered_map<string, struct group_detail *> groups;

int server_socket;
int error_status = 0;

void create_user(vector<string> ip_c, int socket)
{
	string username = ip_c[1];
	if (users.find(username) == users.end())
	{
		struct user_detail *user = new struct user_detail;
		char msg[128];
		user->pwd = ip_c[2];
		user->user_ip = ip_c[3];
		user->user_port = ip_c[4];
		string success = "New Account created as :-" + username;
		users[username] = user;
		strcpy(msg, "Account created successfully!!");
		cout << success << endl;
		send(socket, msg, strlen(msg), 0);
	}
	else
	{
		char msg[128];
		strcpy(msg, "Not Allowed because User already exist!");
		send(socket, msg, strlen(msg), 0);
		return;
	}
}

pair<string, int> login(vector<string> ip_c, int socket)
{
	pair<string, int> p;
	string username = ip_c[1];
	string password = ip_c[2];
	if (users.find(username) == users.end())
	{
		char msg[512];
		p.first = "invalid";
		p.second = 0;
		strcpy(msg, "Username doesn't exist....Please Create first");
		send(socket, msg, strlen(msg), 0);
		return p;
	}

	if (users[username]->pwd == password)
	{
		if (login_status[username] != 1)
		{
			login_status[username] = 1;
			char msg[512];
			p.first = username;
			p.second = 1;
			string success = "Login done Successfully by :-" + username;
			strcpy(msg, "Successfully logged in");
			send(socket, msg, strlen(msg), 0);
			cout << success << endl;
		}
		else
		{
			char msg[512];
			p.first = "invalid";
			p.second = 0;
			strcpy(msg, "User already logged in.");
			send(socket, msg, strlen(msg), 0);
		}
	}
	else
	{
		char msg[512];
		p.first = "invalid";
		p.second = 0;
		strcpy(msg, "Invalid login credentials provided!");
		send(socket, msg, strlen(msg), 0);
	}
	return p;
}

void join_group(vector<string> ip_c, string present_user, int socket)
{
	string groupname = ip_c[1];
	if (groups.find(groupname) == groups.end())
	{
		char msg[512];
		strcpy(msg, "Group doesn't exist at all!");
		send(socket, msg, strlen(msg), 0);
	}
	else
	{
		int f = 0;
		struct group_detail *gd = groups[groupname];
		int len = gd->grp_members.size();
		for (int i = 0; i < len; i++)
			if (gd->grp_members[i] == present_user)
			{
				f = 1;
				break;
			}

		if (f == 0)
		{
			int len = gd->join_reqs.size();
			for (int i = 0; i < len; i++)
				if (gd->join_reqs[i] == present_user)
				{
					f = 1;
					break;
				}

			if (f == 0)
			{
				groups[groupname]->join_reqs.push_back(present_user);
				char msg[512];
				strcpy(msg, "Group join request sent successfully.");
				send(socket, msg, strlen(msg), 0);
			}
			else
			{
				char msg[512];
				strcpy(msg, "Your request is already in pending...Please Wait.");
				send(socket, msg, strlen(msg), 0);
			}
		}
		else
		{
			char msg[512];
			strcpy(msg, "You are already member of this group.");
			send(socket, msg, strlen(msg), 0);
		}
	}
}

void create_group(vector<string> ip_c, string present_user, int socket)
{
	string group_name = ip_c[1];
	if (groups.find(group_name) == groups.end())
	{
		struct group_detail *gd = new struct group_detail[1];
		gd->grp_members.push_back(present_user);
		gd->grp_owner = present_user;
		gd->group_id = group_name;
		groups[group_name] = gd;
		string success = "New Group created as :-" + group_name;
		struct user_detail *ud = users[present_user];
		ud->user_grp.push_back(group_name);
		char msg[512];
		strcpy(msg, "Group successfully created.");
		send(socket, msg, strlen(msg), 0);
		cout << success << endl;
	}
	else
	{
		char msg[512];
		strcpy(msg, "This Group is already exist!");
		send(socket, msg, strlen(msg), 0);
	}
}

void leave_group(vector<string> ip_c, string present_user, int socket)
{
	string groupname = ip_c[1];
	if (groups.find(groupname) == groups.end())
	{
		char msg[512];
		strcpy(msg, "Group doesn't exist at all!");
		send(socket, msg, strlen(msg), 0);
	}
	else
	{
		int f = -1;
		struct group_detail *gd = groups[groupname];
		int len = gd->grp_members.size();
		for (int i = 0; i < len; i++)
			if (gd->grp_members[i] == present_user)
			{
				f = i;
				break;
			}
		if (f < 0)
		{
			char msg[512];
			strcpy(msg, "You are not member of that group.");
			send(socket, msg, strlen(msg), 0);
		}
		else
		{
			if (gd->grp_owner == present_user)
			{
				char msg[512];
				strcpy(msg, "You are group owner. Group owner can't leave group!");
				send(socket, msg, strlen(msg), 0);
			}
			else
			{
				for (auto fi : users[present_user]->user_file)
				{
					struct file_detail *fd = files[map_file[fi]];
					int len = fd->file_owners.size();
					if (len == 1)
					{
						map_file.erase(fi);
						files.erase(map_file[fi]);
						free(fd);
					}
					else
					{
						int len = fd->file_owners.size();
						for (int i = 0; i < len; i++)
						{
							if (fd->grp[i] == groupname && fd->file_owners[i] == present_user)
							{
								int cnt = 0;
								fd->grp.erase(fd->grp.begin() + i);
								fd->file_owners.erase(fd->file_owners.begin() + i);
								len = fd->file_owners.size();
								fd->file_path.erase(fd->file_path.begin() + i);
								for (int j = 0; j < len; j++)
								{
									int x = fd->file_path[j].size() - 1;
									for (; x >= 0; i--)
									{
										if (fd->file_path[j][x] == '/')
										{
											break;
										}
									}
									string gd = fd->file_path[j].substr(x + 1);
									if (fi == gd)
									{
										cnt = 1;
										break;
									}
								}
								if (cnt == 0)
									map_file.erase(fi);
							}
						}
					}
				}
				char msg[512];
				gd->grp_members.erase(gd->grp_members.begin() + f);
				strcpy(msg, "You have leaved group successfully");
				strcat(msg, groupname.c_str());
				send(socket, msg, strlen(msg), 0);
			}
		}
	}
}

void list_requests(vector<string> ip_c, string present_user, int socket)
{
	string groupname = ip_c[1];
	if (groups.find(groupname) == groups.end())
	{
		char msg[2048];
		strcpy(msg, "Group doesn't exist at all!");
		send(socket, msg, strlen(msg), 0);
	}
	else
	{
		struct group_detail *gd = groups[groupname];
		if (present_user != gd->grp_owner)
		{
			char msg[2048];
			strcpy(msg, "Only group owner can list requests.");
			send(socket, msg, strlen(msg), 0);
		}
		else
		{
			char msg[2048];
			string tmp = "Join request list in " + groupname + " :-";
			strcpy(msg, tmp.c_str());
			int len = gd->join_reqs.size();
			for (int i = 0; i < len; i++)
			{
				strcat(msg, "\n");
				strcat(msg, gd->join_reqs[i].c_str());
			}
			if (len > 0)
			{
				send(socket, msg, strlen(msg), 0);
			}
			else
			{
				strcpy(msg, "There is no pending requests present.");
				send(socket, msg, strlen(msg), 0);
			}
		}
	}
}

void accept_request(vector<string> ip_c, string present_user, int socket)
{
	string groupname = ip_c[1];
	string username = ip_c[2];
	if (groups.find(groupname) == groups.end())
	{
		char msg[512];
		strcpy(msg, "Group doesn't exist at all!");
		send(socket, msg, strlen(msg), 0);
	}
	else
	{
		int flag = -1;
		struct group_detail *gd = groups[groupname];
		if (present_user != gd->grp_owner)
		{
			char msg[512];
			strcpy(msg, "Only group owner can accept join request.");
			send(socket, msg, strlen(msg), 0);
		}
		else
		{
			int len = gd->join_reqs.size();
			for (int i = 0; i < len; i++)
				if (username == gd->join_reqs[i])
				{
					flag = i;
					break;
				}

			if (flag >= 0)
			{
				gd->join_reqs.erase(gd->join_reqs.begin() + flag);
				gd->grp_members.push_back(username);
				struct user_detail *ud = users[username];
				ud->user_grp.push_back(groupname);
				char msg[512];
				strcpy(msg, "Join request accepted.");
				send(socket, msg, strlen(msg), 0);
			}
			else
			{
				char msg[512];
				strcpy(msg, "No such join request found!");
				send(socket, msg, strlen(msg), 0);
			}
		}
	}
}

int logout(string present_user, int socket)
{
	login_status[present_user] = 0;
	char msg[512];
	strcpy(msg, "logged out successfully.");
	string success = "Logout done Successfully by user :-" + present_user;
	cout << success << endl;
	send(socket, msg, strlen(msg), 0);
	return 0;
}

void list_groups(int socket)
{
	char msg[1024];
	bzero(msg, 1024);
	strcpy(msg, "Group list:");
	int total_grp = groups.size();
	for (auto i : groups)
	{
		strcat(msg, "\n");
		strcat(msg, i.first.c_str());
	}
	if (total_grp != 0)
	{
		send(socket, msg, strlen(msg), 0);
	}
	else
	{
		strcpy(msg, "Sorry, No group exist!");
		send(socket, msg, strlen(msg), 0);
	}
}

void upload_file(vector<string> ip_c, string present_user, int socket)
{
	//upload_file <filepath> <groupId> <hash> <size>
	string filepath = ip_c[1];
	string groupname = ip_c[2];
	string hash = ip_c[3];
	string f_size = ip_c[4];
	if (groups.find(groupname) == groups.end())
	{
		char msg[512];
		strcpy(msg, "Group doesn't exist at all!");
		send(socket, msg, strlen(msg), 0);
	}
	else
	{
		int f = 0;
		struct user_detail *ud = users[present_user];
		int len = ud->user_grp.size();
		for (int i = 0; i < len; i++)
			if (groupname == ud->user_grp[i])
			{
				f = 1;
				break;
			}

		if (f == 0)
		{
			char msg[512];
			strcpy(msg, "Sorry, You are not in this group!");
			send(socket, msg, strlen(msg), 0);
		}
		else
		{
			int i;
			for (i = filepath.size() - 1; i >= 0; i--)
				if (filepath[i] == '/')
					break;
			string temp = filepath.substr(i + 1);
			if (files.find(hash) == files.end())
			{
				struct file_detail *fd = new file_detail;
				char msg[512];
				fd->total_size = f_size;
				fd->file_owners.push_back(present_user);
				fd->grp.push_back(groupname);
				fd->file_path.push_back(filepath);
				files[hash] = fd;
				users[present_user]->user_file.push_back(temp);
				map_file[temp] = hash;
				strcpy(msg, "File successfully uploaded.");
				send(socket, msg, strlen(msg), 0);
			}
			else
			{
				if (map_file.find(temp) == map_file.end())
				{
					map_file[temp] = hash;
					struct file_detail *fd = files[hash];
					fd->file_owners.push_back(present_user);
					fd->file_path.push_back(filepath);
					fd->grp.push_back(groupname);
					users[present_user]->user_file.push_back(temp);
					char msg[512];
					strcpy(msg, "File successfully uploaded.");
					send(socket, msg, strlen(msg), 0);
				}
				else
				{
					struct file_detail *fd = files[hash];
					int len = fd->file_path.size();
					int f = 0;
					for (int i = 0; i < len; i++)
						if (filepath == fd->file_path[i] && fd->file_owners[i] == present_user)
						{
							f = i;
							break;
						}
					if (f != 0)
					{
						char msg[512];
						strcpy(msg, "File already exist!");
						send(socket, msg, strlen(msg), 0);
					}
					else
					{
						char msg[512];
						strcpy(msg, "File successfully uploaded.");
						send(socket, msg, strlen(msg), 0);
						fd->file_path.push_back(filepath);
						fd->file_owners.push_back(present_user);
						fd->grp.push_back(groupname);
					}
				}
			}
		}
	}
}

void list_files(vector<string> ip_c, int socket)
{
	string groupname = ip_c[1];
	char temp[512];
	if (groups.find(groupname) == groups.end())
	{
		char msg[51200];
		strcpy(msg, "Group doesn't exist at all!");
		send(socket, msg, strlen(msg), 0);
	}
	else
	{
		int x = map_file.size();
		if (x == 0)
		{
			char msg[51200];
			strcpy(msg, "No files exist!");
			send(socket, msg, strlen(msg), 0);
		}
		else
		{
			char msg[51200];
			strcpy(msg, "File list:");
			int count = 0;
			for (auto i : map_file)
			{
				struct file_detail *fd = files[i.second];
				int len = fd->grp.size();
				int f = 0;
				strcpy(temp, i.first.c_str());
				strcat(temp, " ");

				for (int j = 0; j < len; j++)
				{
					string tmp = fd->file_owners[j];
					if (fd->grp[j] == groupname && login_status[tmp])
					{
						f++;
						strcat(temp, fd->file_owners[j].c_str());
						strcat(temp, " ");
					}
				}
				if (f != 0)
				{
					strcat(msg, "\n");
					strcat(msg, temp);
					count++;
				}
			}
			if (count == 0)
			{
				strcpy(msg, "No files exist!");
				send(socket, msg, strlen(msg), 0);
			}
			else
			{
				send(socket, msg, strlen(msg), 0);
			}
		}
	}
}

void download_file(vector<string> ip_c, string present_user, int socket)
{
	// download_file​ <group_id> <file_name> <destination_path>
	string groupname = ip_c[1];
	string filename = ip_c[2];
	string des_path = ip_c[3];
	char msg[51200];
	if (groups.find(groupname) == groups.end())
	{
		strcpy(msg, "Group doesn't exist at all!");
		send(socket, msg, strlen(msg), 0);
	}
	else
	{
		if (map_file.find(filename) == map_file.end())
		{
			strcpy(msg, "File doesn't exist!");
			send(socket, msg, strlen(msg), 0);
		}
		else
		{
			int f = 0;
			struct user_detail *ud = users[present_user];
			int len = ud->user_grp.size();
			for (int i = 0; i < len; i++)
				if (groupname == ud->user_grp[i])
				{
					f = 1;
					break;
				}

			if (f == 0)
			{
				strcpy(msg, "You are not part of this group!");
				send(socket, msg, strlen(msg), 0);
			}
			else
			{
				string temp;
				struct file_detail *fd = files[map_file[filename]];
				temp = fd->total_size + "/" + map_file[filename];
				strcpy(msg, temp.c_str());
				send(socket, msg, strlen(msg), 0);
				sleep(0.1);
				strcpy(msg, "");
				len = fd->file_owners.size();
				for (int i = 0; i < len; i++)
				{
					if (groupname == fd->grp[i] && login_status[fd->file_owners[i]] == 1)
					{
						temp = fd->file_owners[i] + " ";
						temp += users[fd->file_owners[i]]->user_port + " ";
						temp += users[fd->file_owners[i]]->user_ip + " ";
						temp += fd->file_path[i] + "\n";
						strcat(msg, temp.c_str());
					}
				}
				send(socket, msg, strlen(msg), 0);
			}
		}
	}
}

void *handle_request(void *all_socket)
{
	bool login_flag = 0;
	int socket = *(int *)all_socket;
	string present_user;

	while (true)
	{
		vector<string> ip_c;
		string str;
		char msg[51200];
		bzero(msg, 51200);
		recv(socket, msg, sizeof(msg), 0);
		string commmand(msg);
		stringstream s_stream(commmand);
		while (s_stream >> str)
		{
			ip_c.push_back(str);
		}
		string user_cmd = ip_c[0];
		int ip_len = ip_c.size();

		if (user_cmd == "create_user")
		{
			if (ip_len == 5)
			{
				create_user(ip_c, socket);
			}
			else
			{
				char msg[512];
				strcpy(msg, "Correct way to Run this is: create_user​ <user_id> <password>");
				send(socket, msg, strlen(msg), 0);
			}
		}
		else if (user_cmd == "login")
		{
			pair<string, int> p;
			p.first = "invalid";
			p.second = 0;
			if (ip_len == 3)
			{
				p = login(ip_c, socket);
			}
			else
			{
				char msg[512];
				strcpy(msg, "Correct way to Run this is: login <user_id> <password>");
				send(socket, msg, strlen(msg), 0);
			}
			if (p.second != 0)
			{
				present_user = p.first;
				login_flag = 1;
			}
		}
		else if (login_flag == 1)
		{
			if (user_cmd == "create_group")
			{
				if (ip_len == 2)
				{
					create_group(ip_c, present_user, socket);
				}
				else
				{
					char msg[512];
					strcpy(msg, "Correct way to Run this is: create_group​ <group_id>");
					send(socket, msg, strlen(msg), 0);
				}
			}
			else if (user_cmd == "join_group")
			{
				if (ip_len == 2)
				{
					join_group(ip_c, present_user, socket);
				}
				else
				{
					char msg[512];
					strcpy(msg, "Correct way to Run this is: join_group​ <group_id>");
					send(socket, msg, strlen(msg), 0);
				}
			}
			else if (user_cmd == "leave_group")
			{
				if (ip_len == 2)
				{
					leave_group(ip_c, present_user, socket);
				}
				else
				{
					char msg[512];
					strcpy(msg, "Correct way to Run this is: leave_group​ <group_id>");
					send(socket, msg, strlen(msg), 0);
				}
			}
			else if (user_cmd == "list_requests")
			{
				if (ip_len == 2)
				{
					list_requests(ip_c, present_user, socket);
				}
				else
				{
					char msg[512];
					strcpy(msg, "Correct way to Run this is: list_requests <group_id>");
					send(socket, msg, strlen(msg), 0);
				}
			}
			else if (user_cmd == "accept_request")
			{
				if (ip_len == 3)
				{
					accept_request(ip_c, present_user, socket);
				}
				else
				{
					char msg[512];
					strcpy(msg, "Correct way to Run this is: accept_request​ <group_id> <user_id>");
					send(socket, msg, strlen(msg), 0);
				}
			}
			else if (user_cmd == "list_groups")
			{
				if (ip_len == 1)
				{
					list_groups(socket);
				}
				else
				{
					char msg[512];
					strcpy(msg, "Correct way to Run this is: list_groups");
					send(socket, msg, strlen(msg), 0);
				}
			}
			else if (user_cmd == "list_files")
			{
				if (ip_len == 2)
				{
					list_files(ip_c, socket);
				}
				else
				{
					char msg[512];
					strcpy(msg, "Correct way to Run this is: list_files​ <group_id>");
					send(socket, msg, strlen(msg), 0);
				}
			}
			else if (user_cmd == "upload_file")
			{
				if (ip_len == 5)
				{
					upload_file(ip_c, present_user, socket);
				}
				else
				{
					char msg[512];
					strcpy(msg, "Correct way to Run this is: upload_file​ <file_path> <group_id>");
					send(socket, msg, strlen(msg), 0);
				}
			}
			else if (user_cmd == "download_file")
			{
				if (ip_len == 4)
				{
					download_file(ip_c, present_user, socket);
				}
				else
				{
					char msg[512];
					strcpy(msg, "Correct way to Run this is: download_file​ <group_id> <file_name> <destination_path>");
					send(socket, msg, strlen(msg), 0);
				}
			}
			else if (user_cmd == "logout")
			{
				if (ip_len == 1)
				{
					login_flag = logout(present_user, socket);
				}
				else
				{
					char msg[512];
					strcpy(msg, "Correct way to Run this is: logout");
					send(socket, msg, strlen(msg), 0);
				}
			}
			// else if (user_cmd == "show_downloads")
			// {
			// 	if (ip_len != 1)
			// 		cout << "Correct way to Run this is: Show_downloads" << endl;
			// 	else
			// 		show_downloads(commmand);
			// }
			// else if (user_cmd == "stop_share")
			// {
			// 	if (ip_len != 3)
			// 		cout << "Correct way to Run this is: stop_share <group_id> <file_name>" << endl;
			// 	else
			// 		stop_share(commmand);
			// }
			else
			{
				char msg[512];
				strcpy(msg, "Invalid commmand!");
				send(socket, msg, strlen(msg), 0);
			}
		}
		else
		{
			char msg[512];
			strcpy(msg, "Please login. If you don't have account? register.");
			send(socket, msg, strlen(msg), 0);
		}
	}
}

void *as_server(void *p)
{

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
	{
		error_status = 1;
		cout << "Error occured while opening socket." << endl;
		return NULL;
	}
	struct sockaddr_in server_add;
	int *tmp_var = (int *)p;
	int port = *tmp_var;
	bzero((char *)&server_add, sizeof(server_add));
	server_add.sin_port = htons(port);
	server_add.sin_addr.s_addr = INADDR_ANY;
	server_add.sin_family = AF_INET;
	int x = bind(server_socket, (struct sockaddr *)&server_add, sizeof(server_add));
	if (x < 0)
	{
		error_status = 1;
		cout << "Binding is not successful." << endl;
		return NULL;
	}
	listen(server_socket, 1000);
	cout << "Tracker started succesfully" << endl;
	int all_socket[1000];
	int i = 0;
	while (true)
	{
		struct sockaddr_in address_client;
		socklen_t address_length = sizeof(address_client);
		pthread_t client;
		all_socket[i] = accept(server_socket, (struct sockaddr *)&address_client, &address_length);

		if (all_socket < 0)
			cout << "Error occured while accepting Request!" << endl;

		pthread_create(&client, NULL, handle_request, &all_socket[i]);
		cout << "Client is connected successfully!" << endl;
		i = (i + 1) % 1000;
	}

	close(server_socket);
}

void assign_port(int p, int &port)
{
	if (p == 1)
		port = 8001;
	else if (p == 2)
		port = 8002;
	else
	{
		cout << "Tracker_no is not valid. It should be either 1 0r 2:" << endl;
	}
}

int main(int argc, char const *argv[])
{
	if (argc != 3)
	{
		cout << "Correct way to Run this is: ./tracker​ tracker_info.txt ​ tracker_no" << endl;
		return 0;
	}
	const char *arg2 = argv[2];
	int p = atoi(arg2);
	int port;
	pthread_t t;
	string input_str;
	assign_port(p, port);
	pthread_create(&t, NULL, as_server, (void *)&port);
	if (error_status == 1)
		return 0;
	while (true)
	{
		cout << "Enter command:";
		cin >> input_str;
		if (input_str == "quit")
		{
			cout << "Tracker is closing..." << endl;
			pthread_kill(t, SIGUSR1);
			break;
		}
		else
		{
			cout << "Invalid command!" << endl;
		}
	}
	return 0;
}
