#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFLEN 256

// TEACA BOGDAN

int main(int argc, char *argv[]){
	char nume_log[BUFLEN];
	memset(nume_log, 0 , BUFLEN);

	sprintf(nume_log, "client-%d.log", getpid());

	FILE * fisier_log = fopen(nume_log, "w");
	fclose(fisier_log);

	int sesiune_deschisa = 0;
	int asteapta_confirmare = 0;
	int trimite_parola = 0;
	int tcp_sockfd, udp_sockfd, n;

	struct sockaddr_in serv_addr, udp_serv_addr, temp_addr;
	struct hostent * server;

	char buffer[BUFLEN];
	char temp[BUFLEN];

	if(argc < 3){
		fprintf(stderr, "Usage: %s server_address server_port\n", argv[0]);

		exit(0);
	}  
    
	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(tcp_sockfd < 0){
		printf("-10 : Eroare la apel socket\n");

		fisier_log = fopen(nume_log, "a");
		fprintf(fisier_log, "-10 : Eroare la apel socket\n");
		fclose(fisier_log);
	}
    
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &serv_addr.sin_addr);

	udp_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(udp_sockfd < 0){
		printf("-10 : Eroare la apel socket\n");

		fisier_log = fopen(nume_log, "a");
		fprintf(fisier_log, "-10 : Eroare la apel socket\n");
		fclose(fisier_log);
	}
    
	udp_serv_addr.sin_family = AF_INET;
	udp_serv_addr.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &udp_serv_addr.sin_addr);

	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax = 10;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	if(connect(tcp_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		printf("-10 : Eroare la apel connect\n");

		fisier_log = fopen(nume_log, "a");
		fprintf(fisier_log, "-10 : Eroare la apel connect\n");
		fclose(fisier_log);
	}

	FD_SET(tcp_sockfd, &read_fds);
	FD_SET(udp_sockfd, &read_fds);
	FD_SET(0, &read_fds);
    
	while(1){
		fdmax;
		tmp_fds = read_fds;
		if(select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1){
			printf("-10 : Eroare la apel select\n");

			fisier_log = fopen(nume_log, "a");
			fprintf(fisier_log, "-10 : Eroare la apel select\n");
			fclose(fisier_log);
		}

		//multiplexarea conexiunii
		int i;
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if(i == tcp_sockfd){  // TCP
					memset(buffer, 0, BUFLEN);

					if ((n = recv(i, buffer, sizeof(buffer), 0)) > 0) {
						printf("IBANK> %s\n", buffer);

						fisier_log = fopen(nume_log, "a");
						fprintf(fisier_log, "IBANK> %s\n", buffer);
						fclose(fisier_log);

						if(strncmp(buffer, "Welcome", 7) == 0){
							sesiune_deschisa = 1;
						}else if(strncmp(buffer, "Deconectare", 11) == 0){
							sesiune_deschisa = 0;
						}else if(strncmp(buffer, "Transfer realizat cu succes", 20) == 0){
							asteapta_confirmare = 0;
						}else if(strncmp(buffer, "Transfer", 8) == 0){
							asteapta_confirmare = 1;
						}else if(strncmp(buffer, "-9", 2) == 0){
							asteapta_confirmare = 0;
						}
					}else{
						close(tcp_sockfd);
						return 0;
					}
				}else if(i == udp_sockfd){ // UDP
					memset(buffer, 0, BUFLEN);
					int len;
					recvfrom(udp_sockfd, buffer, BUFLEN, 0, (struct sockaddr*)&udp_serv_addr, &len);

					printf ("UNLOCK> %s\n", buffer);

					fisier_log = fopen(nume_log, "a");
					fprintf(fisier_log, "UNLOCK> %s\n", buffer);
					fclose(fisier_log);

					if(strncmp(buffer, "Trimite", 7) == 0){
						trimite_parola = 1;
					}else{
						trimite_parola = 0;
					}
				}else if(i == 0) {  // Trimitere comenzi
					int numar_card;

				    	memset(buffer, 0 , BUFLEN);
				    	fgets(buffer, BUFLEN - 1, stdin);

					fisier_log = fopen(nume_log, "a");
					fprintf(fisier_log, "%s\n", buffer);
					fclose(fisier_log);

					memset(temp, 0 , BUFLEN);
					strcpy(temp, buffer);

					char * parametru = strtok(temp, " ");

					if(asteapta_confirmare == 0 && trimite_parola == 0){
						if(strncmp(parametru, "login", 5) == 0){
							if(sesiune_deschisa == 1){
								printf("-2 : Sesiune deja deschisa\n");

								fisier_log = fopen(nume_log, "a");
								fprintf(fisier_log, "-2 : Sesiune deja deschisa\n");
								fclose(fisier_log);

								continue;
							}else{
								parametru = strtok(NULL, " ");
								numar_card = atoi(parametru);
							}
						}else if(strncmp(parametru, "logout", 6) == 0){
							if(sesiune_deschisa == 0){
								printf("-1 : Clientul nu este autentificat\n");

								fisier_log = fopen(nume_log, "a");
								fprintf(fisier_log, "-1 : Clientul nu este autentificat\n");
								fclose(fisier_log);

								continue;
							}
						}else if(strncmp(parametru, "listsold", 8) == 0){
							if(sesiune_deschisa == 0){
								printf("-1 : Clientul nu este autentificat\n");

								fisier_log = fopen(nume_log, "a");
								fprintf(fisier_log, "-1 : Clientul nu este autentificat\n");
								fclose(fisier_log);

								continue;
							}
						}else if(strncmp(parametru, "transfer", 8) == 0){
							if(sesiune_deschisa == 0){
								printf("-1 : Clientul nu este autentificat\n");

								fisier_log = fopen(nume_log, "a");
								fprintf(fisier_log, "-1 : Clientul nu este autentificat\n");
								fclose(fisier_log);

								continue;
							}
						}else if(strncmp(parametru, "unlock", 6) != 0 && strncmp(parametru, "quit", 4) != 0){
							continue;
						}
					}

					if(strncmp(parametru, "quit", 4) == 0){
						n = send(tcp_sockfd, buffer, strlen(buffer), 0);

					    	if(n < 0){
							printf("-10 : Eroare la apel send\n");

							fisier_log = fopen(nume_log, "a");
							fprintf(fisier_log, "-10 : Eroare la apel send\n");
							fclose(fisier_log);
						}

						close(tcp_sockfd);

						return 0;
					}else if(strncmp(parametru, "unlock", 6) == 0){
						memset(temp, 0 , BUFLEN);
						sprintf(buffer, "unlock %d", numar_card);

						printf("%s\n", buffer);

						sendto(udp_sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&udp_serv_addr, sizeof(udp_serv_addr));
					}else{
						if(trimite_parola == 0){
							n = send(tcp_sockfd, buffer, strlen(buffer), 0);

						    	if(n < 0){
								printf("-10 : Eroare la apel send\n");

								fisier_log = fopen(nume_log, "a");
								fprintf(fisier_log, "-10 : Eroare la apel send\n");
								fclose(fisier_log);
							}
						}else{
							memset(temp, 0 , BUFLEN);
							sprintf(temp, "%d %s", numar_card, buffer);
							strcpy(buffer, temp);

							printf("%s\n", buffer);

							n = sendto(udp_sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&udp_serv_addr, sizeof(udp_serv_addr));

							if(n < 0){
								printf("-10 : Eroare la apel sendto\n");

								fisier_log = fopen(nume_log, "a");
								fprintf(fisier_log, "-10 : Eroare la apel sendto\n");
								fclose(fisier_log);
							}
						}
					}
				}
			}
		}
	}

	close(tcp_sockfd);

	return 0;
}


