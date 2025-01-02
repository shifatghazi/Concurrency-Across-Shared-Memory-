#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <unistd.h>
// #include <fcntl.h> // Used in part A, but not needed for part B
#include <sys/shm.h>
#include <time.h>

#define totalNumMarkers 5 // Number of Teaching Assistants
#define numStudents 20     // Total number of students

// Generate a identifier key from a file and poject ID for the shared memory
#define SHM_KEY 12345  

// Array of semaphores to help TAs take turns marking
sem_t taSemaphores[totalNumMarkers];

void markExams(int taID, int *listOfStudents) {

    //Unique seed is performed by reinitialzing from a random number generator
    srand(time(NULL) + taID);

    int sessionComp = 0;

    while (sessionComp < 3) { // Each TA performs 3 sessions
        for (int i = 0; i < numStudents; i++) {
            // Locking the semaphor for the current and next ta
            sem_wait(&taSemaphores[taID]);
            sem_wait(&taSemaphores[(taID + 1) % totalNumMarkers]);

            int studentID = listOfStudents[i];
            if (studentID == 9999) { // Last student must be 9999
                sem_post(&taSemaphores[(taID + 1) % totalNumMarkers]);
                sem_post(&taSemaphores[taID]);
                break;
            }

            // Random mark given to the student
            int mark = rand() % 11; // Random mark between 0 and 10
            printf("TA %d gave Student %d grade of %d\n", taID + 1, studentID, mark);

            // Delay is performed
            sleep(1 + rand() % 3);

            // Semaphores are now released
            sem_post(&taSemaphores[(taID + 1) % totalNumMarkers]);
            sem_post(&taSemaphores[taID]);
        }
        sessionComp++;
    }
}

int main() {
    pid_t taProcesses[totalNumMarkers]; // The array to store process IDs due to unique tas
    srand(time(NULL)); // Random number generator is initialized

    // Initializing semaphores
    for (int i = 0; i < totalNumMarkers; i++) {
        sem_init(&taSemaphores[i], 1, 1); // Binary semaphore
    }

    //Createing a shared memory segment with a unique key that has been defined by incorporating IPC_CREAT to ensure
    //the segment is created, and 0666 to set read/write permissions for all users

    int shmID = shmget(SHM_KEY, numStudents * sizeof(int), IPC_CREAT | 0666);
    if (shmID < 0) {
        perror("Unable to make shared memory");
        exit(1);
    }

    // Attach to shared memory as requested in part B
    int *listOfStudents = (int *)shmat(shmID, NULL, 0);
    if (listOfStudents == (int *)-1) {
        perror("Shared memory cannot be attached");
        exit(1);
    }

    // Initialize the student list in shared memory
    for (int i = 0; i < numStudents - 1; i++) {
        listOfStudents[i] = 1000 + i;
    }
    //Creating the desired list
    listOfStudents[numStudents - 1] = 9999;

    // Fork processes for each TA
    for (int i = 0; i < totalNumMarkers; i++) {
        taProcesses[i] = fork();
        if (taProcesses[i] == 0) { // Check if this is the child process created by fork()
            markExams(i, listOfStudents);
            exit(0);
        }
    }

    // We must wait for the TA processes to complete
    for (int i = 0; i < totalNumMarkers; i++) {
        waitpid(taProcesses[i], NULL, 0);
    }

    // Fully remove from memory
    shmdt(listOfStudents);
    shmctl(shmID, IPC_RMID, NULL);

    // Cleaning up semaphores
    for (int i = 0; i < totalNumMarkers; i++) {
        sem_destroy(&taSemaphores[i]);
    }

    printf("Marking is complete!\n");
    return 0;
}
