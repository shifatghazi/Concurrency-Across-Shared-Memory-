#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

//Uniqye number of ta's
#define totalNumMarkers 5 

// Array of semaphores to help TAs take turns marking
sem_t taSemaphores[totalNumMarkers];

// Function that will have the tas mark exams
void runMarking(int markerID) {
    // The student list file is opened to read from
    FILE *listOfStudents = fopen("database.txt", "r");
    if (!listOfStudents) {
        perror("database.txt cannot be opened");
        exit(1);
    }

    // Output file is created
    char makeFileName[20];
    sprintf(makeFileName, "TA%d.txt", markerID + 1); // e.g., Marker1.txt
    FILE *marker_file = fopen(makeFileName, "w");
    if (!marker_file) {
        perror("Could not create marker output file");
        fclose(listOfStudents);
        exit(1);
    }

    char line[10];
    int session = 0;

    // Each marker goes through the student list 3 times
    while (session < 3) {
        rewind(listOfStudents); // This will rest to the beginning

        while (fgets(line, sizeof(line), listOfStudents)) {
            int idOfStudent;
            sscanf(line, "%d", &idOfStudent);
            if (idOfStudent == 9999) break; // Last student must be 9999

            // Locking the semaphor for the current and next ta
            sem_wait(&taSemaphores[markerID]);
            sem_wait(&taSemaphores[(markerID + 1) % totalNumMarkers]);

            // Random mark given to the student
            int markGiven = rand() % 11;

            //Output to correct format
            fprintf(marker_file, "Student %d: Mark %d\n", idOfStudent, markGiven);
            printf("Marker %d assigned %d to Student %d\n", markerID + 1, markGiven, idOfStudent);

            // Semaphores are now released
            sem_post(&taSemaphores[(markerID + 1) % totalNumMarkers]);
            sem_post(&taSemaphores[markerID]);

            sleep(1 + rand() % 3); // Simulate marking delay
        }
        session++;
    }

    fclose(listOfStudents);
    fclose(marker_file);
}

int main() {
    pid_t ta_pids[totalNumMarkers];
    srand(time(NULL)); // Random number generator is initialized

    // Initialize semaphores
    for (int i = 0; i < totalNumMarkers; i++) {
        sem_init(&taSemaphores[i], 1, 1); // Binary semaphore being used
    }

    // Forking processes for each marker
    for (int i = 0; i < totalNumMarkers; i++) {
        ta_pids[i] = fork();
        if (ta_pids[i] == 0) { // Child process
            runMarking(i);
            exit(0);
        }
    }

    // Wait for all marker processes to complete
    for (int i = 0; i < totalNumMarkers; i++) {
        waitpid(ta_pids[i], NULL, 0);
    }

    // Cleaning up semaphores
    for (int i = 0; i < totalNumMarkers; i++) {
        sem_destroy(&taSemaphores[i]);
    }

    printf("Marking is complete\n");
    return 0;
}