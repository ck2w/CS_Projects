#include "gfclient-student.h"

#define MAX_THREADS (2105)
int steque_len = 0;

steque_t* request_steque;

char *server = "localhost";
unsigned short port = 30605;

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
  
  char *workload_path = "workload.txt";
  int option_char = 0;
  int nrequests = 5;
  int nthreads = 8;
  char *req_path = NULL;  
  int i = 0;

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

  pthread_t* threads = threadspool_create(nthreads);

  /*Making the requests...*/
  
  for(i = 0; i < nrequests; i++){
    req_path = workload_get_path();

    if(strlen(req_path) > 1024){
      fprintf(stderr, "Request path exceeded maximum of 500 characters\n.");
      exit(EXIT_FAILURE);
    }
    
    pthread_mutex_lock(&mutex);
    
    while (steque_len >= BUFSIZE){            
      pthread_cond_wait(&c_prod, &mutex);      
    }

    steque_enqueue(request_steque, (void *)req_path);
    steque_len++;
    pthread_cond_signal(&c_cons);
    pthread_mutex_unlock(&mutex);

  }
  // posion pill
  pthread_mutex_lock(&mutex);   
  req_path = "a";
  steque_enqueue(request_steque, (void *) req_path);
  steque_len++;
  pthread_cond_broadcast(&c_cons);
  pthread_mutex_unlock(&mutex);

  for (i = 0; i < nthreads; ++i) {
    pthread_join(threads[i], NULL);
  }

  free(threads);

  gfc_global_cleanup();

  return 0;
}


pthread_t* threadspool_create(int nthreads){

  request_steque = malloc(sizeof(steque_t));
  steque_init(request_steque);

  pthread_cond_init(&c_cons, NULL);
  pthread_cond_init(&c_prod, NULL);

  pthread_mutex_init(&mutex, NULL);

  pthread_t* threads;

  threads = malloc(sizeof(pthread_t) * nthreads);
  for (int i = 0; i < nthreads; ++i) {    
    pthread_create(&threads[i], NULL, handle_request_file, NULL);          
  }

  return threads;
}


void* handle_request_file(void* arg){

  gfcrequest_t *gfr = NULL;
  FILE *file = NULL;
  char *req_path = NULL;
  int returncode = 0;
  char local_path[1066];

  for(;;){
    pthread_mutex_lock(&mutex);

    while (steque_len == 0){      
      pthread_cond_wait(&c_cons, &mutex);
    }
    req_path = (char*)steque_pop(request_steque);
    if(*req_path=='a'){
      // poison pill
      steque_enqueue(request_steque, (void *)req_path);
      pthread_mutex_unlock(&mutex);
      pthread_exit(NULL);
    }

    steque_len--;

    pthread_cond_signal(&c_prod);
    pthread_mutex_unlock(&mutex);

    localPath(req_path, local_path);

    file = openFile(local_path);

    gfr = gfc_create();
    gfc_set_server(&gfr, server);
    gfc_set_path(&gfr, req_path);
    gfc_set_port(&gfr, port);
    gfc_set_writefunc(&gfr, writecb);
    gfc_set_writearg(&gfr, file);

    fprintf(stdout, "Requesting %s%s\n", server, req_path);

    if ( 0 > (returncode = gfc_perform(&gfr))){
      fprintf(stdout, "gfc_perform returned an error %d\n", returncode);
      fclose(file);
      if ( 0 > unlink(local_path))
        fprintf(stderr, "warning: unlink failed on %s\n", local_path);
    }
    else {
      fclose(file);
    }

    if ( gfc_get_status(&gfr) != GF_OK){
      if ( 0 > unlink(local_path))
        fprintf(stderr, "warning: unlink failed on %s\n", local_path);
    }

    fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(&gfr)));
    fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr), gfc_get_filelen(&gfr));

    gfc_cleanup(&gfr);
  }

  return NULL;
}