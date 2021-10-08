#include <vector>
#include <fstream>

class Server
{
  public: // later move variables to private (public for ease of testing)
    vector<bool> bitmap;    // keeps track of empty lines to handle the PUT request
    fstream f_db;           // handles the key-value db in persistent storage
    fstream f_log;          // handles the log file

    Server()
    {
        // init
    }
    // declare methods to handle clients

    // declare methods to service the requests and respond
};
