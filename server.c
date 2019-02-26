#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_CLIENTS 20
#define BUFLEN 256

// TEACA BOGDAN

typedef struct Utilizator{
	char * nume;
	char * prenume;
	int numar_card;
	int pin;
	char * parola;
	double sold;

	//folosit in cadrul transferurilor si are initial valoarea
	//-1, iar dupa ce utilizatorul respectiv da comanda de transfer catre un alt cont,
	//variabila "asteapta_confimare" ia valoarea numarului de card al utilizatorului
	//catre care se face transferul pentru a usura gasirea utilizatorului destinatie
	//dupa confirmarea transferului.
	int asteapta_confirmare;

	//retine valoarea sumei care sa fie transferata dupa confirmarea
	//comenzii catre utilizatorul destinatie (in cadrul comenzii "transfer")
	double transfer_suma;

	//variabila cu rol boolean, 0 = card neblocat, 1 = card blocat
	int card_blocat;

	//file_descriptor-ul corespunzator utilizatorului respectiv, in cadrul sesiunii curente
	int fd;

	//numarul de incercari esuate consecutive pentru logare (la a 3-a
	//incercare esuata se blocheaza cardul)
	int incercari;

	//valoarea 0 = niciun proces client nu a facut o cerere de
	//deblocare,  valoarea 1 = a fost data deja o cerere pentru deblocare, are rol
	//de a nu lasa urmatoarele procese client care cer deblocarea aceluiasi card
	//simultan (conform enuntului, numai primul proces client poate trimite parola
	//cardului, ceilalti primesc codul de eroare -6).
	int cerere_deblocare;
} Utilizator;

int main(int argc, char * argv[]){
	int n, i, j;

	FILE * file = fopen(argv[2], "r");

	int numar_utilizatori;

	fscanf(file, "%d", &numar_utilizatori);

	Utilizator * users = (Utilizator *)malloc(numar_utilizatori * sizeof(Utilizator));

	for(i = 0; i < numar_utilizatori; i++){
		users[i].nume = (char *)malloc(13 * sizeof(char));
		users[i].prenume = (char *)malloc(13 * sizeof(char));
		users[i].parola = (char *)malloc(9 * sizeof(char));

		users[i].asteapta_confirmare = -1;
		users[i].card_blocat = 0;
		users[i].fd = -1;
		users[i].incercari = 0;
		users[i].cerere_deblocare = 0;

		fscanf(file, "%s %s %d %d %s %lf\n", users[i].nume, users[i].prenume, &users[i].numar_card, &users[i].pin, users[i].parola, &users[i].sold);

		printf("%s, %s, %d, %d, %s, %.2f\n", users[i].nume, users[i].prenume, users[i].numar_card, users[i].pin, users[i].parola, users[i].sold);
	}

	fclose(file);

	int user_index[100];

	for(i = 0; i < 100; i++){
		user_index[i] = -1;
	}

	int tcp_sockfd, newsockfd, portno, clilen, udp_sockfd;
	char buffer[BUFLEN];
	char temp[BUFLEN];
	struct sockaddr_in serv_addr, udp_serv_addr, cli_addr, temp_addr;

	fd_set read_fds; //multimea de citire folosita pentru select()
	fd_set tmp_fds; //multime folosita temporar 
	int fdmax; //valoare maxima file descriptor din multimea read_fds

	if(argc < 3){
		fprintf(stderr, "Usage: %s port file_name\n", argv[0]);
		exit(1);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
     
	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(tcp_sockfd < 0){
		printf("-10 : Eroare la apel socket\n");
	}
     
	portno = atoi(argv[1]);

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
     
	if(bind(tcp_sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0){
		printf("-10 : Eroare la apel bind\n");
	}
     
	listen(tcp_sockfd, MAX_CLIENTS);

	udp_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(udp_sockfd < 0){
		printf("-10 : Eroare la apel socket\n");
	}

	memset((char *) &udp_serv_addr, 0, sizeof(udp_serv_addr));
	udp_serv_addr.sin_family = AF_INET;
	udp_serv_addr.sin_addr.s_addr = INADDR_ANY;
	udp_serv_addr.sin_port = htons(portno);

	if(bind(udp_sockfd, (struct sockaddr *)&udp_serv_addr, sizeof(struct sockaddr)) < 0){
		printf("-10 : Eroare la apel bind\n");
	}

	FD_SET(tcp_sockfd, &read_fds);
	FD_SET(udp_sockfd, &read_fds);
	FD_SET(0, &read_fds);

	if(tcp_sockfd > udp_sockfd){
		fdmax = tcp_sockfd;
	}else{
		fdmax = udp_sockfd;
	}

	while(1){
		tmp_fds = read_fds; 
		if(select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1){
			printf("-10 : Eroare la apel select\n");
		}
	
		//multiplexarea conexiunii
		for(i = 0; i <= fdmax; i++) {
			if(FD_ISSET(i, &tmp_fds)){
				if(i == 0){
					memset(buffer, 0 , BUFLEN);
				    	fgets(buffer, BUFLEN - 1, stdin);

					printf("%s\n", buffer);

					if(strncmp(buffer, "quit", 4) == 0){
						close(tcp_sockfd);
						return 0;
					}
				}else if(i == tcp_sockfd){ // Gestionare conexiuni noi TCP
					clilen = sizeof(cli_addr);

					if((newsockfd = accept(tcp_sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1){
						printf("-10 : Eroare la apel accept\n");
					}else{
						FD_SET(newsockfd, &read_fds);

						if(newsockfd > fdmax){ 
							fdmax = newsockfd;
						}
					}

					printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				}else if(i == udp_sockfd){ // Gestionare comenzi UDP (pentru Unlock)
					memset(buffer, 0, BUFLEN);
					int len;
					recvfrom(udp_sockfd, buffer, BUFLEN, 0, (struct sockaddr*)&temp_addr, &len);

					printf ("UDP: %s\n", buffer);

					memset(temp, 0 , BUFLEN);
					strcpy(temp, buffer);

					char * parametru = strtok(temp, " ");

					if(strncmp(parametru, "unlock", 6) == 0){
						parametru = strtok(NULL, " ");
						int numar_card = atoi(parametru);

						printf("%d\n", numar_card);

						int card_inexistent = 0;

						int k;
						for(k = 0; k < numar_utilizatori; k++){
							if(users[k].numar_card == numar_card){
								if(users[k].cerere_deblocare == 1){
									memset(buffer, 0 , BUFLEN);
									strcpy(buffer, "-7 : Deblocare esuata\n");
								}else if(users[k].card_blocat == 1){
									memset(buffer, 0 , BUFLEN);
									strcpy(buffer, "Trimite parola secreta\n");

									users[k].cerere_deblocare = 1;
								}else{
									memset(buffer, 0 , BUFLEN);
									strcpy(buffer, "-6 : Operatie esuata\n");
								}

								break;
							}

							if(k == numar_utilizatori - 1){
								card_inexistent = 1;
							}
						}

						if(card_inexistent == 1){
							memset(buffer, 0 , BUFLEN);
							strcpy(buffer, "-4 : Card inexistent\n");
						}

						int x = sendto(udp_sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&temp_addr, sizeof(temp_addr));

						if(x < 0){
							printf("-10 : Eroare la apel sendto\n");
						}
					}else{
						int numar_card = atoi(parametru);

						char * parola = strtok(NULL, " ");

						int k;
						for(k = 0; k < numar_utilizatori; k++){
							if(users[k].numar_card == numar_card){
								if(strncmp(users[k].parola, parola, strlen(users[k].parola)) == 0){
									users[k].card_blocat = 0;

									memset(buffer, 0 , BUFLEN);
									strcpy(buffer, "Card deblocat\n");

									users[k].incercari = 0;
									users[k].cerere_deblocare = 0;
								}else{
									memset(buffer, 0 , BUFLEN);
									strcpy(buffer, "-7 : Deblocare esuata\n");

									users[k].cerere_deblocare = 0;
								}

								break;
							}
						}

						int x = sendto(udp_sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&temp_addr, sizeof(temp_addr));

						if(x < 0){
							printf("-10 : Eroare la apel sendto\n");
						}
					}
				}else{ // Gestionare comenzi TCP
					memset(buffer, 0, BUFLEN);

					if((n = recv(i, buffer, sizeof(buffer), 0)) <= 0){
						if(n == 0){
							printf("Server: Socket %d quit\n", i); //conexiunea cu acest client s-a terminat
						}else{
							printf("-10 : Eroare la apel recv\n");
						}

						close(i);

						FD_CLR(i, &read_fds);
					}else { //recv intoarce >0
						fflush(stdout);
						printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);
						fflush(stdout);

						memset(temp, 0 , BUFLEN);
						strcpy(temp, buffer);

						char * parametru = strtok(temp, " ");

						if(strncmp(parametru, "quit", 4) == 0){
							if(user_index[i] != -1){
								users[user_index[i]].asteapta_confirmare = -1;
								users[user_index[i]].fd = -1;
								user_index[i] = -1;
							}
						}else if(user_index[i] == -1 || users[user_index[i]].asteapta_confirmare == -1){
							if(strncmp(parametru, "login", 5) == 0){
								parametru = strtok(NULL, " ");
								int numar_card = atoi(parametru);

								parametru = strtok(NULL, " ");
								int pin = atoi(parametru);

								int card_inexistent = 0;

								int k;
								for(k = 0; k < numar_utilizatori; k++){
									if(users[k].numar_card == numar_card){
										if(users[k].card_blocat == 1){
											memset(buffer, 0 , BUFLEN);
											strcpy(buffer, "-5 : Card blocat\n");
										}else if(users[k].pin == pin){
											if(users[k].fd == -1){
												users[k].card_blocat = 0;
												users[k].incercari = 0;
												users[k].cerere_deblocare = 0;

												users[k].fd = i;
												user_index[i] = k;

												memset(buffer, 0 , BUFLEN);
												sprintf(buffer, "Welcome %s %s", users[k].nume, users[k].prenume);
											}else{
												memset(buffer, 0 , BUFLEN);
												strcpy(buffer, "-2 : Sesiune deja deschisa\n");
											}
										}else{
											users[k].incercari++;

											if(users[k].incercari == 3){
												users[k].card_blocat = 1;

												memset(buffer, 0 , BUFLEN);
												strcpy(buffer, "-5 : Card blocat\n");
											}else{
												memset(buffer, 0 , BUFLEN);
												strcpy(buffer, "-3 : Pin gresit\n");
											}
										}

										break;
									}

									if(k == numar_utilizatori - 1){
										card_inexistent = 1;
									}
								}

								if(card_inexistent == 1){
									memset(buffer, 0 , BUFLEN);
									strcpy(buffer, "-4 : Card inexistent\n");
								}

							    	//trimit mesaj la client
							    	n = send(i, buffer, strlen(buffer), 0);

							    	if(n < 0){
									 printf("-10 : Eroare la apel send\n");
								}
							}else if(strncmp(parametru, "logout", 6) == 0){
								if(users[user_index[i]].fd == i){
									users[user_index[i]].fd = -1;
									user_index[i] = -1;

									memset(buffer, 0 , BUFLEN);
									strcpy(buffer, "Deconectare de la bancomat");
								}

								n = send(i, buffer, strlen(buffer), 0);

							    	if(n < 0){
									 printf("-10 : Eroare la apel send\n");
								}
							}else if(strncmp(parametru, "listsold", 8) == 0){
								if(users[user_index[i]].fd == i){
									memset(buffer, 0 , BUFLEN);
									sprintf(buffer, "%.2f", users[user_index[i]].sold);
								}

								n = send(i, buffer, strlen(buffer), 0);

							    	if(n < 0){
									printf("-10 : Eroare la apel send\n");
								}
							}else if(strncmp(parametru, "transfer", 8) == 0){
								if(users[user_index[i]].fd == i){
									parametru = strtok(NULL, " ");
									int numar_card = atoi(parametru);

									printf("%d\n", numar_card);

									parametru = strtok(NULL, " ");
									double suma = atof(parametru);

									printf("%lf\n", suma);

									int card_inexistent = 0;

									int m;
									for(m = 0; m < numar_utilizatori; m++){
										if(users[m].numar_card == numar_card){
											if(users[user_index[i]].sold < suma){
												memset(buffer, 0 , BUFLEN);
												strcpy(buffer, "-8 : Fonduri insuficiente\n");
											}else{
												users[user_index[i]].asteapta_confirmare = m;
												users[user_index[i]].transfer_suma = suma;

												memset(buffer, 0 , BUFLEN);
												sprintf(buffer, "Transfer %.2f catre %s %s? [y/n]", suma, users[m].nume, users[m].prenume);
											}

											break;
										}

										if(m == numar_utilizatori - 1){
											card_inexistent = 1;
										}
									}

									if(card_inexistent == 1){
										memset(buffer, 0 , BUFLEN);
										strcpy(buffer, "-4 : Card inexistent\n");
									}
								}

								printf("%s\n", buffer);

								n = send(i, buffer, strlen(buffer), 0);

							    	if(n < 0){
									 printf("-10 : Eroare la apel send\n");
								}
							}
						}else{
							if(strncmp(parametru, "y", 1) == 0){
								if(users[user_index[i]].fd == i){
									users[user_index[i]].sold -= users[user_index[i]].transfer_suma;
									users[users[user_index[i]].asteapta_confirmare].sold += users[user_index[i]].transfer_suma;

									users[user_index[i]].asteapta_confirmare = -1;

									memset(buffer, 0 , BUFLEN);
									strcpy(buffer, "Transfer realizat cu succes\n");
								}

								n = send(i, buffer, strlen(buffer), 0);

							    	if(n < 0){
									 printf("-10 : Eroare la apel send\n");
								}
							}else{
								if(users[user_index[i]].fd == i){
									users[user_index[i]].asteapta_confirmare = -1;

									memset(buffer, 0 , BUFLEN);
									strcpy(buffer, "-9 : Operatie anulata\n");
								}

								n = send(i, buffer, strlen(buffer), 0);

							    	if(n < 0){
									 printf("-10 : Eroare la apel send\n");
								}
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


