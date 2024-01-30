using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
#include <string.h>

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

/* Method Prototypes */
film create_film(string title, int year);
void get_movie_info(char* &info, int offset, film &film);
void check_divisible_by_four(int &num_bytes, char* &info);
void check_is_even(int &num_bytes, char* &info);

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

/* file struct, which consists of const void* (aka actorFile) and string name*/
struct file{
  const void* file;
  string name;
};

/* movie_file struct, which consists of const void* (aka movieFile) and film movie*/
struct movie_file{
  const void* file;
  film movie;
};

/* Comparison method for getCredits method */
int compare_Fn(const void* keyStruct, const void* compare)
{
  file* actor_struct = (file*)(keyStruct);
  int offset = *(int*)compare;
  char* second_str = (char*)actor_struct->file + offset;
  string str2(second_str);
  if(actor_struct->name < str2){
    return -1;
  }else if(actor_struct->name == str2){
    return 0;
  }else{
    return 1;
  }
}

/* Comparison method for getCast method */
int compare_fn(const void* keyStruct, const void* compare){
  movie_file* film_struct = (movie_file*)keyStruct;
  int compare_offset = *(int*)compare;
  film compare_film;
  char* film_info = (char*)film_struct->file;
  get_movie_info(film_info, compare_offset, compare_film);
  film key = film_struct->movie;
  if(key < compare_film){
    return -1;
  }else if(key == compare_film){
    return 0;
  }else{
    return 1;
  }
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}
 
bool imdb::getCredits(const string& player, vector<film>& films) const 
{
  file key;
  key.file = actorFile;
  key.name = player;
  int* result = (int*)bsearch((const void*)&key, (const void*)((int*)actorFile), *(int*)actorFile, sizeof(int), compare_Fn);
  if(result == NULL){
    return false;
  }
  int actor_offset = *result;
  //moving to info about actor by counting actor_offset number of bytes from actorFile
  char* actor_info = (char*)actorFile + actor_offset;
  //number of bytes needed to encode information
  int num_bytes = 0;
  //skipping actor's name and null character
  num_bytes += (strlen(actor_info) + 1);
  actor_info += (strlen(actor_info) + 1); 
  check_is_even(num_bytes, actor_info);
  //getting number of movies actor has played in
  short num_movies = *(short*)actor_info; 
  check_divisible_by_four(num_bytes, actor_info);
  for(int i = 0; i < (int)num_movies; i++){
    //pointer arithmetic
    int movie_offset = *(int*)(actor_info + i * sizeof(int));
    film newFilm;
    char* movie_info = (char*)movieFile;
    get_movie_info(movie_info, movie_offset, newFilm);
    films.push_back(newFilm);
  }
  return true;
}

/* Checks if number of bytes is divisible by 4 to make sure to have four-byte offsets for integers */
void check_divisible_by_four(int &num_bytes, char* &info){
  num_bytes += sizeof(short);
  if(num_bytes % 4 == 0){
    info += 2;
  }else{
    info += 4;
  }
}

/* checking if number of bytes is odd or even to do action correspondingly */
void check_is_even(int &num_bytes, char* &info){
  if(num_bytes % 2 != 0){
    num_bytes++;
    info++;
  }
}

/* Creates film with given title and year delta */
film create_film(string title, int year){
  film newFilm;
  newFilm.title = title;
  newFilm.year = 1900 + year;
  return newFilm;
}

/* Gets information about the movie and creates new film */
void get_movie_info(char* &info, int offset, film &film){
  char* movie_info = info + offset;
  string title(movie_info);
  movie_info += (strlen(movie_info) + 1);
  int year = (int)*movie_info;
  film = create_film(title, year);
}

bool imdb::getCast(const film& movie, vector<string>& players) const 
{
  movie_file file_struct;
  file_struct.file = movieFile;
  file_struct.movie = movie;
  int* result = (int*)bsearch((const void*)&file_struct, (const void*)((int*)movieFile + 1), *(int*)movieFile, sizeof(int), compare_fn);
  if(result == NULL){
    return false;
  }
  int movie_offset = *result;
  //moving to info about movie by counting actor_offset number of bytes from movieFile
  char* movie_info = (char*)movieFile + movie_offset;
  //number of bytes needed to encode information
  int num_bytes = 0;
  //skipping movie's name, null character and one byte of year
  num_bytes += (strlen(movie_info) + 2);
  movie_info += (strlen(movie_info) + 2);
  check_is_even(num_bytes, movie_info);
  //getting number of actors in this movie
  short num_actors = *(short*)movie_info;
  check_divisible_by_four(num_bytes, movie_info);
  for(int i = 0; i < (int)num_actors; i++){
    //pointer arithmetic
    int actor_offset = *(int*)(movie_info + i * sizeof(int));
    char* actor_name = (char*)actorFile + actor_offset;
    string name(actor_name);
    players.push_back(name);
  }  
  return true;
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}