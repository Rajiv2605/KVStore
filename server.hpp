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
        f_db.open("keydb.txt", ios::out | ios::app | ios::in);
        f_log.open("log.txt", ios::out | ios::app | ios::in);
    }

    ~Server()
    {
      f_db.close();
      f_log.close();
    }

    // commit logs for the given key
    // void commit_logs(string key);

    // appending to db
    // void write_put(string key, string value, int index);
    
    // deleting from DB
    // void write_del(string key, int index);

    // Readers-Writer lock for logfile handling
    // void db_lock();
    // void db_unlock();
    // void log_lock();
    // void log_unlock();

    // declare methods to handle clients
    // void receive();
    // void respond();

    // declare methods to service the requests and respond
    void handle_get(string key);
    void handle_put(string key, string value);
    void handle_delete(string key);
    // void write_cache();
};
