#include <vector>
#include <fstream>

using namespace std;

class Server
{
  public: // later move variables to private (public for ease of testing)
    vector<bool> bitmap;    // keeps track of empty lines to handle the PUT request
    fstream f_db;           // handles the key-value db in persistent storage
    fstream f_log;          // handles the log file

    Server()
    {
        // init
        f_db.open("keydb.txt", ios::app | ios::in);
        f_log.open("log.txt", ios::app | ios::in);
    }

    ~Server()
    {
      f_db.close();
      f_log.close();
    }

    // Readers-Writer lock for logfile handling
    void lock();
    void unlock();

    // declare methods to handle clients
    void receive();
    void respond();

    // declare methods to service the requests and respond
    void handle_get(string key);
    void handle_put(string key, string value);
    void handle_delete(string key);
    void write_cache();
};
