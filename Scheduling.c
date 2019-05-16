#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

//////////////////////////////////////////////////
////////////////////////////////////////////////// globals
//////////////////////////////////////////////////

#define JOB_COUNT_MAX 100
#define END_TIME 100000

char *jobsFilename = "jobs.dat";

enum JobStatus {
    UNKNOWN,
    RUNNABLE,
    RUNNING,
    DONE
};

typedef struct {
    // job specs
    int id;
    int arrivalTime;
    int duration;
    // scheduler data
    enum JobStatus status;
    int startTime;
    int endTime;
    int timeRunning;     // time spent running so far
    int timeLeft;        // time left to run, duration - timeRunning
    int lastStartedTime; // last started by scheduler
} Job;

//////////////////////////////////////////////////
////////////////////////////////////////////////// functions
//////////////////////////////////////////////////

int readJobs(char *filename, Job *jobs, int maxCount) {
    FILE *f = fopen(filename, "r");
    if(f == NULL) {
        printf(
            "ERROR: cannot open file [%s] for reading: %s\n",
            filename,
            strerror(errno)
        );
        exit(EXIT_FAILURE);
    }

    int i;
    for(
        i = 0;
        (
            fscanf(
                f,
                "%d%d%d",
                &jobs[i].id,
                &jobs[i].arrivalTime,
                &jobs[i].duration
            ) != EOF
        ) &&
        (i < maxCount);
        i++
    ) {
        jobs[i].status    = UNKNOWN;
        jobs[i].startTime = -1;
        jobs[i].endTime   = -1;
    }

    return i;
}

int allJobsDone(Job *jobs, int jobCount) {
    for(int i = 0; i < jobCount; i++) {
        if(jobs[i].status != DONE) {
            return 0; // something is not done
        }
    }

    return 1; // all done
}

int printRunLog(Job *jobs, int jobCount, char *schedulerType) {
    printf("Run log for %s:\n", schedulerType);
    for(int i = 0; i < jobCount; i++) {
        Job *j = &jobs[i];
        printf(
            "Job id %02d start/finish %02d - %02d, total %02d, response %02d\n",
            j->id,
            j->startTime,
            j->endTime,
            j->endTime - j->arrivalTime,
            j->startTime - j->arrivalTime
        );
    }

    return 1; // all done
}

Job *runningJob(Job *jobs, int jobCount) {
    for(int i = 0; i < jobCount; i++) {
        Job *j = &jobs[i];
        if(j->status == RUNNING) {
            return j;
        }
    }

    return NULL;
}

// marks finished running job (if any) as DONE
void markFinishedJob(Job *jobs, int jobCount, int ticker) {
    Job *j = runningJob(jobs, jobCount);
    if((j != NULL) && (j->endTime <= ticker)) {
        j->status  = DONE;
        j->endTime = ticker;
    }
}

// first in first out, no preemption
Job *chooseJobFIFO(Job *jobs, int jobCount, int ticker) {

    int minArrivalTime = INT_MAX;
    Job *chosenJob      = NULL;

    for(int i = 0; i < jobCount; i++) {
        Job *j = &jobs[i];
        if(j->status == RUNNING) { // no preemption
            chosenJob = NULL;
            break;
        }
        if((j->status == RUNNABLE) && (j->arrivalTime < minArrivalTime)) {
            minArrivalTime = j->arrivalTime;
            chosenJob     = j;
        }
    }

    return chosenJob;
}

// short job first, no preemption
Job *chooseJobSJF(Job *jobs, int jobCount, int ticker) {

    int minDuration = INT_MAX;
    Job *chosenJob  = NULL;

    for(int i = 0; i < jobCount; i++) {
        Job *j = &jobs[i];
        if(j->status == RUNNING) { // no preemption
            chosenJob = NULL;
            break;
        }
        if((j->status == RUNNABLE) && (j->duration < minDuration)) {
            minDuration = j->duration;
            chosenJob   = j;
        }
    }

    return chosenJob;
}

// biggest job first, no preemption
Job *chooseJobBJF(Job *jobs, int jobCount, int ticker) {

    int maxDuration = -1;
    Job *chosenJob  = NULL;

    for(int i = 0; i < jobCount; i++) {
        Job *j = &jobs[i];
        if(j->status == RUNNING) { // no preemption
            chosenJob = NULL;
            break;
        }
        if((j->status == RUNNABLE) && (j->duration > maxDuration)) {
            maxDuration = j->duration;
            chosenJob   = j;
        }
    }

    return chosenJob;
}

// shortest time to completion first
Job *chooseJobSTCF(Job *jobs, int jobCount, int ticker) {

    int minTimeLeft = INT_MAX;
    Job *chosenJob  = NULL;

    for(int i = 0; i < jobCount; i++) {
        Job *j = &jobs[i];
        if((j->status == RUNNABLE) && (j->timeLeft < minTimeLeft)) {
            minTimeLeft = j->timeLeft;
            chosenJob   = j;
        }
    }

    return chosenJob;
}

// round robin
Job *chooseJobRR(Job *jobs, int jobCount, int ticker) {

    static int rrSlice = 3;

    Job *running = runningJob(jobs, jobCount);

    if(running == NULL) {
        for(int i = 0; i < jobCount; i++) {
            Job *j = &jobs[i];
            if(j->status == RUNNABLE) {
                return j;
            }
        }
        return NULL;
    } else {
        if((ticker - running->lastStartedTime) >= rrSlice) {
            // time to get somebody else a chance, next available id
            // starting with running job id
            for(int i = running->id; i < jobCount; i++) {
                Job *j = &jobs[i];
                if(j->status == RUNNABLE) {
                    return j;
                }
            }
            for(int i = 0; i < running->id; i++) {
                Job *j = &jobs[i];
                if(j->status == RUNNABLE) {
                    return j;
                }
            }
            // nobody else is available
            return NULL;
        } else {
            // keep it running
            return NULL;
        }
    }

    return NULL;
}

void run(
    char *schedulerType,
    Job *(*chooseJob)(Job *jobs, int jobCount, int ticker),
    Job *jobs,
    int jobCount
) {
    for(int ticker = 0; ticker < END_TIME; ticker++) {
        for(int i = 0; i < jobCount; i++) {
            Job *j = &jobs[i];
            if((j->arrivalTime == ticker) && (j->status == UNKNOWN)) {
                j->status          = RUNNABLE;
                j->startTime       = -1;
                j->endTime         = -1;
                j->timeRunning     = 0;
                j->timeLeft        = j->duration;
                j->lastStartedTime = -1;
            }
        }

        markFinishedJob(jobs, jobCount, ticker);

        Job *chosenJob = chooseJob(jobs, jobCount, ticker);
        if(chosenJob != NULL) {
            Job *j;
            if((j = runningJob(jobs, jobCount)) != NULL) {
                // move running job aside
                j->status      = RUNNABLE;
                j->timeRunning += j->timeLeft - j->lastStartedTime;
                j->timeLeft    = j->duration - j->timeRunning;
            }
            // start the chosen job
            chosenJob->status          = RUNNING;
            chosenJob->startTime       = (chosenJob->startTime == -1) ?
                ticker : chosenJob->startTime;
            chosenJob->lastStartedTime = ticker;
            chosenJob->endTime         = ticker + chosenJob->timeLeft;
        }

        if(allJobsDone(jobs, jobCount)) {
            break;
        }
    }

    printRunLog(jobs, jobCount, schedulerType);

    return;
}

//////////////////////////////////////////////////
////////////////////////////////////////////////// main
//////////////////////////////////////////////////

int main(void) {
    Job jobs[JOB_COUNT_MAX + 1] = {0};
    int jobCount = readJobs(jobsFilename, jobs, JOB_COUNT_MAX);

    run("FIFO", chooseJobFIFO, jobs, jobCount); // no preemption
    run("SJF",  chooseJobSJF,  jobs, jobCount); // no preemption
    run("BJF",  chooseJobBJF,  jobs, jobCount); // no preemption
    run("STCF", chooseJobSTCF, jobs, jobCount);
    run("RR",   chooseJobRR,   jobs, jobCount);

    return 0;
}
