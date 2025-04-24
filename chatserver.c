//chatserver.c

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h> /* struct timeval を利用するため*/
#include <stdlib.h>
#include <stdbool.h>
#define MAXCLIENTS 5
#define MAXNAMELEN 50
#define BUFLEN 1024

int main(int argc, char **argv)
{

	int clen, sock, csock[MAXCLIENTS + 1], reuse, k, nbytes;
	bool usrname_available;
	int len;
	char rbuf[BUFLEN], msg[BUFLEN];
	char name[MAXCLIENTS][MAXNAMELEN];
	struct sockaddr_in svr, clt;
	fd_set rfds;
	struct timeval tv;

	memset(rbuf, 1, sizeof(rbuf));
	/* s1 */
	/* create the socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror("socket");
		exit(1);
	}

	reuse = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
	{
		perror("setsockopt");
		exit(1);
	}

	bzero(&svr, sizeof(svr));
	svr.sin_family = AF_INET;
	svr.sin_addr.s_addr = htonl(INADDR_ANY);
	svr.sin_port = htons(10140); /* ポート番号10140 を指定して受け付ける*/

	/* ソケットにソケットアドレスを割り当てる*/
	if (bind(sock, (struct sockaddr *)&svr, sizeof(svr)) < 0)
	{
		perror("bind");
		exit(1);
	}

	if (listen(sock, 5) < 0)
	{
		perror("listen");
		exit(1);
	}

	k = 0;

	/* s2 */

	/* 変更を監視するファイル記述子の集合を変数rfds にセットする*/
	while (1)
	{
		FD_ZERO(&rfds);		 /* rfds を初期化*/
		FD_SET(sock, &rfds); /* 接続要求を待つソケット*/
		/* クライアントを受け付けたソケット*/
		for (int i = 0; i < k; i++)
		{
			FD_SET(csock[i], &rfds);
		}
		/* 監視する待ち時間を1 秒に設定*/
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		/* 接続要求とソケットからの送信を同時に監視する*/

		if (select(FD_SETSIZE, &rfds, NULL, NULL, &tv) > 0)
		{
			/* s3 */
			/* 変更があったファイル記述子それぞれに対して処理 */
			if (FD_ISSET(sock, &rfds))
			{
				/* s4 */
				/* クライアントの受付*/
				clen = sizeof(clt);
				if ((csock[k] = accept(sock, (struct sockaddr *)&clt, &clen)) < 0)
				{
					perror("accept");
					exit(2);
				}
				// FD_SET(csock[k],&rfds);
				if (k >= MAXCLIENTS)
				{
					write(csock[k], "REQUEST REJECTED\n", 17);
					close(csock[k]);
				}
				else
				{
					write(csock[k], "REQUEST ACCEPTED\n", 17);
					/* s5 */
					nbytes = read(csock[k], rbuf, sizeof(rbuf));
					for (len = 0; len < MAXNAMELEN; len++)
					{
						if (rbuf[len] == '\n')
						{
							rbuf[len] = '\0';
							break;
						}
					}
					if (len == MAXNAMELEN)
					{
						fprintf(stderr, "Username too long, only registered the available part\n");
						rbuf[len - 1] = '\0';
					}

					usrname_available = true;
					for (int i = 0; i < k; i++)
					{
						if (strcmp(rbuf, name[i]) == 0)
						{
							usrname_available = false;
							break;
						}
					}

					if (usrname_available == false)
					{	
						write(csock[k], "USERNAME REJECTED\n", 18);
						strcat(rbuf, " is rejected.\n");
						write(1, rbuf, strlen(rbuf));
						close(csock[k]);
					}
					else if (usrname_available == true)
					{
						strcpy(name[k], rbuf);
						printf("%s is registered.\n", name[k]);
						write(csock[k], "USERNAME REGISTERED\n", 20);
						k++;
					}
					else
					{
						fprintf(stderr, "unexpected error\n");
						exit(1);
					}
				}
			}

			for (int i = 0; i < k; i++)
			{
				if (FD_ISSET(csock[i], &rfds))
				{
					/* s6 */
					nbytes = read(csock[i], rbuf, sizeof(rbuf));
					if (nbytes == 0)
					{
						/* s7 */
						close(csock[i]);
						printf("%s left the chat.\n", name[i]);
						memset(name[i], 0, sizeof(name[i]));
						for (len=i; len < k - 1; len++)
						{
							csock[len] = csock[len + 1];
							strcpy(name[len], name[len + 1]);
							memset(name[len + 1], 0, sizeof(name[len + 1]));
						}
						k--;
					}
					else
					{
						memset(msg, 0, sizeof(msg));
						strcpy(msg, name[i]);
						strcat(msg, " >");
						if (strlen(msg) + nbytes > BUFLEN - 1)
						{
							fprintf(stderr, "Error: The message is too long to be delivered...\n");
							strncat(msg, rbuf, BUFLEN - 1 - strlen(msg));
						}
						else
						{
							strncat(msg, rbuf, nbytes);
						}
						for (int j = 0; j < k; j++)
						{
							write(csock[j], msg, strlen(msg));
						}
					}
				}
			}
		}
	}
}
