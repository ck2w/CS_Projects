#include "gfclient-student.h"

#define MAX_THREADS (2105)
int num = 0;
int finish_enque = 0;

#define USAGE                                                   \
  "usage:\n"                                                    \
  "  webclient [options]\n"                                     \
  "options:\n"                                                  \
  "  -h                  Show this help message\n"              \
  "  -r [num_requests]   Request download total (Default: 5)\n" \
  "  -p [server_port]    Server port (Default: 30605)\n"        \
  "  -s [server_addr]    Server address (Default: 127.0.0.1)\n" \
  "  -t [nthreads]       Number of threads (Default 8)\n"       \
  "  -w [workload_path]  Path to workload file (Default: workload.txt)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"nthreads", required_argument, NULL, 't'},
    {"nrequests", required_argument, NULL, 'r'},
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"workload-path", required_argument, NULL, 'w'},
    {NULL, 0, NULL, 0}};

static void Usage() { fprintf(stderr, "%s", USAGE); }

static void localPath(char *req_path, char *local_path) {
  static int counter = 0;

  sprintf(local_path, "%s-%06d", &req_path[1], counter++);
}

static FILE *openFile(char *path) {
  char *cur, *prev;
  FILE *ans;

  /* Make the directory if it isn't there */
  prev = path;
  while (NULL != (cur = strchr(prev + 1, '/'))) {
    *cur = '\0';

    if (0 > mkdir(&path[0], S_IRWXU)) {
      if (errno != EEXIST) {
        perror("Unable to create directory");
        exit(EXIT_FAILURE);
      }
    }

    *cur = '/';
    prev = cur;
  }

  if (NULL == (ans = fopen(&path[0], "w"))) {
    perror("Unable to open file");
    exit(EXIT_FAILURE);
  }

  return ans;
}

/* Callbacks ========================================================= */
static void writecb(void *data, size_t data_len, void *arg) {
  FILE *file = (FILE *)arg;

  fwrite(data, 1, data_len, file);
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  /* COMMAND LINE OPTIONS ============================================= */
  char *server = "localhost";
  unsigned short port = 30605;
  char *workload_path = "workload.txt";
  int option_char = 0;
  int nrequests = 5;
  int nthreads = 8;
  // int returncode = 0;
  gfcrequest_t *gfr = NULL;
  FILE *file = NULL;
  char *req_path = NULL;
  char local_path[1066];
  int i = 0;
  request_t *new_request;
  

  setbuf(stdout, NULL);  // disable caching

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:n:hs:w:r:t:", gLongOptions,
                                    NULL)) != -1) {
    switch (option_char) {
      case 'h':  // help
        Usage();
        exit(0);
        break;
      case 'r':
        nrequests = atoi(optarg);
        break;
      case 'n':  // nrequests
        break;
      case 'p':  // port
        port = atoi(optarg);
        break;
      default:
        Usage();
        exit(1);
      case 's':  // server
        server = optarg;
        break;
      case 't':  // nthreads
        nthreads = atoi(optarg);
        break;
      case 'w':  // workload-path
        workload_path = optarg;
        break;
    }
  }

  if (EXIT_SUCCESS != workload_init(workload_path)) {
    fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
    exit(EXIT_FAILURE);
  }

  if (nthreads < 1) {
    nthreads = 1;
  }
  if (nthreads > MAX_THREADS) {
    nthreads = MAX_THREADS;
  }

  gfc_global_init();

  /* add your threadpool creation here */
  printf("create thread %ld\n", sizeof(threadspool));
  threadspool_create(nthreads);


  /* Build your queue of requests here */
  for (i = 0; i < nrequests; i++) {
    /* Note that when you have a worker thread pool, you will need to move this
     * logic into the worker threads */
    req_path = workload_get_path();
    new_request = init_request();

    if (strlen(req_path) > 1024) {
      fprintf(stderr, "Request path exceeded maximum of 1024 characters\n.");
      exit(EXIT_FAILURE);
    }

    localPath(req_path, local_path);

    file = openFile(local_path);

    gfr = gfc_create();
    gfc_set_server(&gfr, server);
    gfc_set_path(&gfr, req_path);
    gfc_set_port(&gfr, port);
    gfc_set_writefunc(&gfr, writecb);
    gfc_set_writearg(&gfr, file);

    fprintf(stdout, "Requesting %s%s\n", server, req_path);

    new_request->gfr = gfr;
    new_request->req_path = req_path;
    new_request->file = file;
    new_request->finish = 0;

    printf("start lock1\n");
    pthread_mutex_lock(&mutex);

    // push
    if (steque_size(&request_queue)>=QUEUESIZE){
      exit(1);
    }
    while(steque_size(&request_queue)==QUEUESIZE){
      pthread_cond_wait(&c_prod, &mutex);
    }
    steque_push(&request_queue, (steque_item) new_request);
    // num++;
    printf("finish push\n");

    if (i == (nrequests-1)){
      finish_enque = 1;
    }

    pthread_mutex_unlock(&mutex);
    printf("end lock1\n");

    pthread_cond_broadcast(&c_cons);
    printf("producer: inserted %d\n", i);





    // if (0 > (returncode = gfc_perform(&gfr))) {
    //   fprintf(stdout, "gfc_perform returned an error %d\n", returncode);
    //   fclose(file);
    //   if (0 > unlink(local_path))
    //     fprintf(stderr, "warning: unlink failed on %s\n", local_path);
    // } else {
    //   fclose(file);
    // }

    // if (gfc_get_status(&gfr) != GF_OK) {
    //   if (0 > unlink(local_path))
    //     fprintf(stderr, "warning: unlink failed on %s\n", local_path);
    // }

    // fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(&gfr)));
    // fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr),
    //         gfc_get_filelen(&gfr));

    // gfc_cleanup(&gfr);

    /*
     * note that when you move the above logic into your worker thread, you will
     * need to coordinate with the boss thread here to effect a clean shutdown.
     */
  }

  // finish_enque = 1;
  // pthread_cond_broadcast(&c_finish);

  // finish event
  // new_request = init_request();
  // new_request->finish = 1;
  // pthread_mutex_lock(&mutex);
  // // push
  // if (num>QUEUESIZE){
  //   exit(1);
  // }
  // while(num==QUEUESIZE){
  //   pthread_cond_wait(&c_prod, &mutex);
  // }
  // steque_push(&request_queue, (steque_item) new_request);
  // num++;
  // printf("finish event push\n");
  // pthread_mutex_unlock(&mutex);
  // pthread_cond_broadcast(&c_cons);

  // pthread_mutex_lock(&mutex);

  // pthread_mutex_unlock(&mutex);

  // pthread_mutex_lock(&finish_mutex);
  // finish_enque = 1;
  // pthread_mutex_unlock(&finish_mutex);  
  // pthread_cond_broadcast(&c_cons);


  for (i = 0; i < nthreads; i++) {
    pthread_join(threadspool[i], NULL);
  }
  printf("finish join\n");



  free(threadspool);
  gfc_global_cleanup();  // use for any global cleanup for AFTER your thread
                         // pool has terminated.

  // pthread_mutex_destroy(&mutex);
  // pthread_cond_destroy(&c_cons);
  // pthread_cond_destroy(&c_prod);
  // pthread_cond_destroy(&c_finish);
  // pthread_exit(NULL);
  printf("finish clean\n");

  return 0;
}


void threadspool_create(int nthreads){
  int rc;
  // create thread pool
  threadspool = malloc(sizeof(pthread_t) * nthreads);
  if(threadspool==NULL){
    perror("malloc");
    printf("threadspool malloc fail\n");
  }
  for(int i; i<nthreads; i++){
    printf("In main: creating thread %d\n", i);
    rc = pthread_create(&threadspool[i], NULL, (void *) handle_request_file, NULL);
    if (rc){
      printf("ERROR; return code from threadspool_create() is %d\n", rc);
      exit(-1);
    }
  }

  // create mutex
  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_init(&finish_mutex, NULL);

  // create condition variable
  pthread_cond_init (&c_cons, NULL);
  pthread_cond_init (&c_prod, NULL);
  pthread_cond_init (&c_finish, NULL);
  printf("finish create %ld\n", sizeof(threadspool));
  
}

request_t *init_request(){
  request_t *r = malloc(sizeof(request_t));
  
  return r;
}

void handle_request_file(){
  printf("start handle\n");
  request_t *request_this;
  gfcrequest_t *gfr_this;
  FILE *file_this;
  int returncode_this = 0;
  char local_path[BUFSIZE];

  for(;;){

    // steque pop
    printf("start lock2\n");
    pthread_mutex_lock(&mutex);   
    
    if (finish_enque==1 && steque_size(&request_queue)==0){
      pthread_mutex_unlock(&mutex);
      break;
    }


    // if(num<0){
    //   printf("num<0 \n");
    //   exit(1);
    // }
    // while(num==0){
    //   pthread_cond_wait(&c_cons, &mutex);
    // }

    // while(num==0){
    //   pthread_mutex_lock(&finish_mutex);
    //   if(finish_enque==1){
    //     pthread_mutex_unlock(&mutex);
    //     pthread_mutex_unlock(&finish_mutex);
    //     pthread_exit(NULL);
    //   }
    //   pthread_mutex_unlock(&finish_mutex);
    //   pthread_cond_wait(&c_cons, &mutex);
    // }
    while(steque_size(&request_queue)==0){
      if(finish_enque==1){
        pthread_mutex_unlock(&mutex);
        pthread_exit(NULL);
      }
      pthread_cond_wait(&c_cons, &mutex);
    }    


    request_this = (request_t *) steque_pop(&request_queue);
    
    // num--;
    pthread_mutex_unlock(&mutex);
    printf("end lock2\n");

    pthread_cond_signal(&c_prod);

    gfr_this = request_this->gfr;
    file_this = request_this->file;
    
    localPath(request_this->req_path, local_path);

    if (0 > (returncode_this = gfc_perform(&gfr_this))) {
      fprintf(stdout, "gfc_perform returned an error %d\n", returncode_this);
      fclose(file_this);
      if (0 > unlink(local_path))
        fprintf(stderr, "warning: unlink failed on %s\n", local_path);
    } 
    else {
      fclose(file_this);
    }

    if (gfc_get_status(&gfr_this) != GF_OK) {
      if (0 > unlink(local_path)) 
      fprintf(stderr, "warning: unlink failed on %s\n", local_path);
    }

    fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(&gfr_this)));
    fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr_this), gfc_get_filelen(&gfr_this));

    gfc_cleanup(&gfr_this);

    printf("exit thread\n");
    printf("num: %d\n", num);
    // pthread_cond_signal(&c_finish);

  }
  // return NULL;

}