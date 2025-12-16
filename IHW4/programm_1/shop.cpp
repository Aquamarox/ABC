#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>

#define DEPARTMENTS 3

using namespace std;

pthread_mutex_t print_mutex;
sem_t dept_sem[DEPARTMENTS];

bool running = true;
ofstream logfile;

/* Universal log function */
void log_msg(const string &msg) {
    pthread_mutex_lock(&print_mutex);
    cout << msg << endl;
    logfile << msg << endl;
    pthread_mutex_unlock(&print_mutex);
}

/* Seller thread */
void *seller_thread(void *arg) {
    int id = *(int *) arg;

    while (running) {
        sem_wait(&dept_sem[id]);
        if (!running) break;

        log_msg("Seller of department " + to_string(id + 1) +
                " is serving a customer");

        sleep(1 + rand() % 3);
    }
    return nullptr;
}

/* Buyer thread */
void *buyer_thread(void *arg) {
    int id = *(int *) arg;

    int visits = 1 + rand() % DEPARTMENTS;
    vector<int> order = {0, 1, 2};
    random_shuffle(order.begin(), order.end());

    log_msg("Customer " + to_string(id) + " entered the store");

    for (int i = 0; i < visits; i++) {
        int dept = order[i];
        log_msg("Customer " + to_string(id) +
                " joined the queue at department " + to_string(dept + 1));

        sem_post(&dept_sem[dept]);
        sleep(1);
    }

    log_msg("Customer " + to_string(id) + " left the store");
    return nullptr;
}

/* Read config file */
int read_config(const char *filename) {
    ifstream file(filename);
    string line;
    int buyers = 0;

    while (getline(file, line)) {
        if (line.find("buyers=") == 0) {
            buyers = atoi(line.substr(7).c_str());
        }
    }
    return buyers;
}

int main(int argc, char *argv[]) {
    int buyers = 0;
    const char *config_file = nullptr;
    const char *output_file = nullptr;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n"))
            buyers = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-c"))
            config_file = argv[++i];
        else if (!strcmp(argv[i], "-o"))
            output_file = argv[++i];
    }

    if (config_file)
        buyers = read_config(config_file);

    if (buyers <= 0 || !output_file) {
        cout << "Usage:\n"
                << "./shop -n <buyers> -o <file>\n"
                << "./shop -c <config> -o <file>\n";
        return 1;
    }

    logfile.open(output_file);
    srand(time(nullptr));

    pthread_mutex_init(&print_mutex, nullptr);
    for (int i = 0; i < DEPARTMENTS; i++)
        sem_init(&dept_sem[i], 0, 0);

    pthread_t sellers[DEPARTMENTS];
    int seller_ids[DEPARTMENTS];

    for (int i = 0; i < DEPARTMENTS; i++) {
        seller_ids[i] = i;
        pthread_create(&sellers[i], nullptr, seller_thread, &seller_ids[i]);
    }

    vector<pthread_t> buyer_threads(buyers);
    vector<int> buyer_ids(buyers);

    for (int i = 0; i < buyers; i++) {
        buyer_ids[i] = i + 1;
        pthread_create(&buyer_threads[i], nullptr,
                       buyer_thread, &buyer_ids[i]);
        usleep(200000);
    }

    for (int i = 0; i < buyers; i++)
        pthread_join(buyer_threads[i], nullptr);

    running = false;
    for (int i = 0; i < DEPARTMENTS; i++)
        sem_post(&dept_sem[i]);

    for (int i = 0; i < DEPARTMENTS; i++)
        pthread_join(sellers[i], nullptr);

    logfile << "Store day is finished\n";
    cout << "Store day is finished\n";

    logfile.close();
    pthread_mutex_destroy(&print_mutex);
    for (int i = 0; i < DEPARTMENTS; i++)
        sem_destroy(&dept_sem[i]);

    return 0;
}
