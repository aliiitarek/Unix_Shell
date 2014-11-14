#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#define Interactive 0
#define Batch 1

const int num_arg = 10, max_size_arg = 100, sizeHistory = 100;
const char* historyPath = "/Users/AlyTarek/Desktop/history.txt"; /*Change this according to running Environment*/
const char* logPath = "/Users/AlyTarek/Desktop/log.txt"; /*Change this according to running Environment*/
char* batchPath;
int argSize, lenPATH;
int mode; /*Interactive->0 & Batch->1*/

/*
 * Checks if the two input
 * strings are the same
 * returns 1 if same
 * returns 0 if different
 */
int areEqual(char* s1, char* s2) {
	int len1 = strlen(s1), len2 = strlen(s2);
	if (len1 != len2)
		return 0;
	int i;
	for (i = 0; i < len1; i++) {
		if (s1[i] != s2[i])
			return 0;
	}
	return 1;
}

/*
 * This method gets the environment
 * variable value recursively
 */
char* getEnvVar(char* envName){
	char* answer;
	answer = (char*) malloc(max_size_arg*(sizeof(char)));
	if(envName[0]=='$'){
		char* envTemp;
		envTemp = (char*) malloc(max_size_arg*(sizeof(char)));
		int idx = 0;
		int i;
		for(i=1;envName[i]!='\0';i++) envTemp[idx++] = envName[i];
		strcpy(envName,envTemp);
	}
	answer = getenv(envName);
	if(answer[0]!='$') {
		return answer;
	}
	return getEnvVar(answer);
}

/*
 * Parses the input command
 * splits on split
 * returns an array of arguments
 *
 * ** also it handles Enviroment variables
 */
char** parser(char* input, char split) {
	if(input[0]=='#') return NULL; // check if comment :: handled by the callee
	char** parsed;
	parsed =(char**)malloc(num_arg * (sizeof(char*)));
	memset(parsed,'\0',num_arg * (sizeof(char*)));
	int i;
	for (i = 0; i < num_arg; i++) {
		parsed[i] =(char*)malloc(max_size_arg * (sizeof(char)));
		memset(parsed[i],'\0',max_size_arg * (sizeof(char)));
	}
	//don't forget to free() each row then all the array
	int arg = 0, ip = 0, c = 0, size_ip = strlen(input);
	while (ip < size_ip) {
		while (ip < size_ip && input[ip] != split && input[ip] != '\t') {
			//if(parsed[arg][c]=='\t'){c++;continue;}
			parsed[arg][c++] = input[ip++];
		}
		while (ip < size_ip && (input[ip] == split || input[ip] == '\t')) {
			ip++;
		}
		if(arg==0){
			char* envVar;
			char* valenv;
			envVar = (char*) malloc(max_size_arg*(sizeof(char)));
			valenv = (char*) malloc(max_size_arg*(sizeof(char)));
			int envvar_idx = 0, valenv_idx = 0;
			for(i=0;parsed[arg][i]!='\0' && parsed[arg][i]!='=';i++) envVar[envvar_idx++] = parsed[arg][i];
			if(parsed[arg][i]!='\0' && parsed[arg][i]=='='){
				for(i=i+1;parsed[arg][i]!='\0';i++) valenv[valenv_idx++] = parsed[arg][i];
				setenv(envVar, valenv, 1);
//				char* enviromentTry = getenv(envVar); // checker
//				printf("%s %s\n",envVar,enviromentTry); // checker
				continue;
			}
		}
		arg++;
		c = 0;
	}
	parsed[arg] = NULL;
	if(split==' '){
		for(i=0;parsed[i]!=NULL;i++){ /*replacing enviroment variables with their values*/
			if(parsed[i][0]=='$') {
				char* envName;
				envName = (char*) malloc(max_size_arg*(sizeof(char)));
				int envName_idx = 0;
				int j;
				for(j=1;parsed[i][j]!='\0';j++) envName[envName_idx++] = parsed[i][j];
				parsed[i] = getEnvVar(envName);
			}
		}
		argSize = arg;
	}
	if(split==':')lenPATH = arg;
	return parsed;
}

/*
 * This method takes the binary files
 * and checks which file has the given
 * command using the access() method
 *
 * returns the path of the command
 * or "Invalid" if can't find command
 *
 */
char* getCommand(char** src, char* command){
	if( access( command, F_OK ) != -1 ) { // this handles the case /bin/ls -l
		return command;
	}
	char* returned;
	returned =(char*)malloc(max_size_arg * (sizeof(char)));
	int found = 0;
	int i;
	for(i=0;src[i]!=NULL;i++){
		char path[100];
		strcpy(path, src[i]);
		strcat(path,"/");
		strcat(path,command);
//		printf("tried %s\n",path);
		if( access( path, F_OK ) != -1 ) {
			strcpy(returned,path);
			found = 1;
			break;
		}
	}
	if(found) return returned;
	return "Invalid";
}

/*
 * This method prints into the log file
 * when a child process terminates
 */
void signalHandler(){
	FILE *filewriter = fopen(logPath, "a");
	fprintf(filewriter, "%s\n", "Child process was terminated");
	fclose(filewriter);
}

/*
 * This method handles modes
 * of the shell
 * Reads a string from stdin
 * Checks if the command exists
 * Excecutes it or handles errors
 */
void brain(){
	char input[520];
	char* PATH = getenv("PATH");
	char** srcs = parser(PATH, ':');
	signal(SIGCHLD,signalHandler);
	FILE *filereaderBat = fopen(batchPath, "r");
	while(1){
		if(mode==Interactive){
			printf("SHELL> ");
			if(fgets(input, 520, stdin)==NULL) break;
		}
		if(mode==Batch){
			if(fgets(input, 520, filereaderBat) == NULL) break;
			printf("%s",input);
		}
		input[strlen(input)-1] = '\0';
		if(input[strlen(input)-2]=='\r')input[strlen(input)-2] = '\0';
		FILE *filewriter = fopen(historyPath, "a");
		fprintf(filewriter, "%s\n", input);
		fclose(filewriter);
		if(strlen(input)>512) {printf("ERROR!! : argument too long\n"); continue;}/*Error argument too long*/

		char** parsed = parser(input,' ');
		if(parsed==NULL) continue; /* Comment --ignore line-- */

		if(argSize==0) continue;
		if(areEqual(parsed[0],"exit") || areEqual(parsed[0],"Exit")) break; /*exit command*/
		if(areEqual(parsed[0],"cd")){
			if(argSize>2) {printf("ERROR!! : invalid arguments to cd\n");continue;} /*Error invalid arguments to cd*/
			if(argSize==2 && parsed[1][0]=='~'){ /* set ~ to /home */
				char tempo[100];
				strcpy(tempo,"/home");
				int tempo_idx = 5;
				int i;
				for(i=1;parsed[1][i]!='\0';i++) tempo[tempo_idx++] = parsed[1][i];
				strcpy(parsed[1],tempo);
			}
			if(argSize==1) {parsed[1] = "/";}
			if(chdir(parsed[1])==-1) perror(""); /*Try handling Documents SHELL>*/
			continue;
		}
		if(areEqual(parsed[0], "history") && parsed[1]==NULL){
			fclose(filewriter);

			//read from history file
			FILE *filereader = fopen(historyPath, "r");
			char command[512];
			while(fgets(command, 520, filereader) != NULL){
				printf("%s", command);
			}
			filewriter = fopen(historyPath, "a");
			continue;
		}
		char* command;
		command =(char*)malloc(max_size_arg * (sizeof(char)));
		command = getCommand(srcs, parsed[0]);
		if(areEqual(command,"Invalid")) {printf("ERROR!! : invalid command\n");continue;} /*Error invalid command*/
		pid_t id = fork();
		if(id==0){ /*Child Process*/
			if(areEqual(parsed[argSize-1],"&")){
				parsed[argSize-1] = '\0';
			}
			char* arguments[max_size_arg];
			int i=0;
			for(;parsed[i]!=NULL;i++) {
				arguments[i] = parsed[i];
			}
			arguments[i] = NULL;
			execv(command,arguments);
		}else{ /*Parent Process*/
			// id contains identification of the child
			if(!areEqual(parsed[argSize-1],"&")){
//				int* idp = &id;
//				wait(idp);
				waitpid(id,NULL,1); /*child id, , 1 -> wait only for this instruction*/
			}
		}
	}
}

/*Main method
 * call from terminal
 * if argument is file the ==> Batch mode
 * else Interactive mode
 * set mode then run the brain() method
 */
int main(int argc, char * argv[]) {
	if(argc==2){ /*Batch mode*/
		mode = Batch;
		batchPath = argv[1];
		if( access( batchPath, F_OK ) == -1 ) {
			perror("Invalid File\n");
			return 0;
		}
	}else{ /*Interactive mode*/
		mode = Interactive;
	}
	brain();
	return 0;
}



