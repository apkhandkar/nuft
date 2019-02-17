#include "message_queue.h"
#include "printmsg.h"
#include <string.h>
#include <stdio.h>

int
main(void)
{
	struct msg mesg;
	char str[128];
	int n;

	int choice = 1;

	char c;

	do {
		printf("Enter a choice:\n");
		printf("1. Add Message\n");
		printf("2. Print Forwards\n");
		printf("3. Print Backwards\n");
		printf("4. Get Index of Message To\n");
		printf("5. Get Contents of Message at Index\n");
		printf("6. Delete Message at Index\n");
		printf("0. Exit\n");

		scanf("%d", &choice);

		if(choice == 1) {
			printf("type: ");
			scanf("%d", &(mesg.type));
			printf("blkno: ");
			scanf("%d", &(mesg.blkno));
			/* absolutely needed because C is a ****ing PITA */
			while((c = getchar()) != '\n' && c != EOF);
			printf("ids: ");
			fgets(mesg.ids, 129, stdin);
			mesg.ids[strlen(mesg.ids)-1] = '\0';
			printf("body: ");
			fgets(mesg.body, 1024, stdin);
			mesg.body[strlen(mesg.body)-1] = '\0';
			add_to_queue(&mesg);	
		} else if(choice == 2) {
			print_queue(0);
		} else if(choice == 3) {
			print_queue(1);
		} else if(choice == 4) {
			while((c = getchar()) != '\n' && c != EOF);
			printf("to: ");
			fgets(str, 128, stdin);
			str[strlen(str)-1] = '\0';
			printf("Index: %d\n", get_mqindex_to(str));
		} else if(choice == 5) {
			printf("index: ");
			scanf("%d", &n);
			printf("\n");
			printmsg(get_message(n));	
			printf("\n");
		} else if(choice == 6) {
			printf("index: ");
			scanf("%d", &n);
			delete_from_queue(n);
		} else {
		}
	} while(choice);

	return 0;
}
