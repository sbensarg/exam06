/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   myserver.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sbensarg <sbensarg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/09/26 18:51:29 by sbensarg          #+#    #+#             */
/*   Updated: 2022/09/27 01:35:31 by sbensarg         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>


typedef struct s_client
{
    int fd;
    int id;
    struct s_client *next;
}   t_client;

t_client *g_clients = NULL;
int server_socket, g_id = 0;
fd_set current_socket, cpy_read, cpy_write;
char msg[42];
char str[42*4096], tmp[42*4096], buf[42*4096 + 42];

void fatal_err()
{
    write(2, "Fatal error\n", strlen("Fatal error\n"));
    close(server_socket);
    exit(1);
}

int max_sockets()
{
    int max = server_socket;
    t_client *tmp = g_clients;

    while (tmp)
    {
        if (tmp->fd > max)
            max = tmp->fd;
        tmp = tmp->next;
    }
    return (max);
        
}

int add_client_to_list(int client_socket)
{
    t_client *tmp = g_clients;
    t_client *new;
    if (!(new = calloc(1, sizeof(t_client))))
        fatal_err();
    new->id = g_id++;
    new->fd = client_socket;
    new->next = NULL;
    if (!g_clients)
        g_clients = new;
    else
    {
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = new;
    }
    return (new->id);
}

void send_msg_to_all_clients(int client_socket, char *msg)
{
    t_client *tmp = g_clients;
    
    while (tmp)
    {
        if (tmp->fd != client_socket && FD_ISSET(tmp->fd, &cpy_write))
        {
            if (send(tmp->fd, msg, strlen(msg), 0) < 0)
                fatal_err();
        }
        tmp = tmp->next;
    }
    
}
int get_id(int fd)
{
    t_client *tmp = g_clients;
    while (tmp)
    {
        if(tmp->fd == fd)
            return (tmp->id);
        tmp = tmp->next;
    }
    return (-1);
}

int rm_client(int fd)
{
    t_client *tmp = g_clients;
    t_client *del;
    int id = get_id(fd);
    if (tmp && tmp->fd == fd)
    {
        g_clients = tmp->next;
        free(tmp);
    }
    else
    {
        while (tmp && tmp->next && tmp->next->fd != fd)
            tmp = tmp->next;
        del = tmp->next;
        tmp->next = tmp->next->next;
        free(del);
    }
    return (id);
}

void add_client()
{
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    int client_socket;
    if ((client_socket = accept(server_socket, (struct sockaddr *)&client_socket, &len)) < 0)
        fatal_err();
    //sprintf stands for “String print”. Instead of printing on console, 
    //it store output on char buffer which are specified in sprintf.
    sprintf(msg, "server: client %d just arrived\n", add_client_to_list(client_socket));
    send_msg_to_all_clients(client_socket, msg);
    FD_SET(client_socket, &current_socket);
    
}

void    ex_msg(int fd)
{
    int i = 0;
    int j = 0;

    while (str[i])
    {
        tmp[j] = str[i];
        j++;
        if (str[i] == '\n')
        {
            sprintf(buf, "client %d: %s", get_id(fd), tmp);
            send_msg_to_all_clients(fd, buf);
            j = 0;
            bzero(&tmp, sizeof(tmp));
            bzero(&buf, sizeof(buf));
             
        }
       i++;
    }
    bzero(&str, sizeof(str));
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
        exit(1);
    }

    // struct for handling internet addresses
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    
    uint16_t port = atoi(argv[1]);
  
    //initialize the adderess struct
    servaddr.sin_family = AF_INET; // AF_INET is an address family that is used to designate the type of addresses that your socket can communicate with (in this case, Internet Protocol v4 addresses)
    servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
    servaddr.sin_port = htons(port);
    
    // socket create and verification 
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        fatal_err();
        
    // Binding newly created socket to given IP and verification 
    if ((bind(server_socket, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        fatal_err();
    
    if (listen(server_socket, 10) != 0)
        fatal_err();
    
    //init my current set
    FD_ZERO(&current_socket);
    //add the file descriptor sockfd to current_socket
    FD_SET(server_socket, &current_socket);

    bzero(&str, sizeof(str));
    bzero(&tmp, sizeof(tmp));
    bzero(&buf, sizeof(buf));

    while (1)
    {
        //because select is destructive
        cpy_read = current_socket;
        cpy_write = current_socket;

        /*  arg 1 : specifies the range of file descriptors to be tested, 
            The select() function tests file descriptors in the range of 0 to nfds-1
            arg 2 : If the {readfds, writefds} argument is not a null pointer, it points to an object
            of type fd_set that on input specifies the file descriptors to be checked for 
            being ready to read, and on output indicates which file descriptors are ready to read.
         */
        if (select(max_sockets() + 1, &cpy_read, &cpy_write, NULL, NULL) < 0)
            continue;
        
        for (int i = 0; i <= max_sockets(); i++)
        {
            if (FD_ISSET(i, &cpy_read))
            {
                if (i == server_socket)
                {
                    // this is a new connection
                    bzero(&msg, sizeof(msg));
                    add_client();
                    break;
                }
                else
                {
                    // do whatever we do with connections
                    int ret_recv = 1000;
                    while(ret_recv == 1000 || str[strlen(str) - 1] != '\n')
                    {
                        ret_recv = recv(i, str + strlen(str), 1000, 0);
                        if (ret_recv <= 0)
                            break;
                    }
                    if (ret_recv <= 0)
                    {
                        bzero(&msg, sizeof(msg));
                        sprintf(msg, "server: client %d just left\n", rm_client(i));
                        send_msg_to_all_clients(i, msg);
                        FD_CLR(i, &current_socket);
                        close(i);
                        break;
                    }
                    else
                    {
                        ex_msg(i);
                    }
                }
            }
        }    
    }
    return (0);
}