#include "server.hpp"
#include <iostream>
#include <sstream>
#include <string>

void Server::handle_get(string key)
{
    string line;
    bool found = false;
    while(f_db)
    {
        getline(f_db, line);
        stringstream getVal(line);
        string k, v;
        getVal>>k;
        getVal>>v;
        if(key==k)
        {
            found = true;
            // respond here
            // update cache
            break;
        }
    }

    if(!found)
        cout<<"KEY NOT FOUND!"<<endl;
}

void Server::handle_put(string key, string value)
{
    stringstream conv(key);
    f_db.seekg(0, ios::beg);
    int K;
    conv>>K;
    string line;
    bool written = false;
    bool isEmpty = f_db.peek() == EOF;
    string newl = key + " " + value;
    if(isEmpty)
    {
        f_db.seekg(0, ios::beg);
        f_db<<newl<<endl;
        return;
    }

    ofstream newdb;
    newdb.open("temp.txt", ofstream::out);

    while(getline(f_db, line))
    {
        stringstream getVals(line);
        int k;
        getVals>>k;
        if(k < K)
            newdb<<line<<endl;
        else if(k > K)
        {
            if(!written)
            {
                newdb<<newl<<endl;
                written = true;
            }

            newdb<<line<<endl;
        }
    }

    if(!written)
        newdb<<newl<<endl;

    newdb.close();
    f_db.close();
    remove("keydb.txt");
    rename("temp.txt", "keydb.txt");
    f_db.open("keydb.txt", ios::out | ios::app | ios::in);
}

void Server::handle_delete(string key)
{
    f_db.seekg(0, ios::beg);
    string line;
    bool found = false;
    ofstream newdb;
    newdb.open("temp.txt", ofstream::out);
    while(getline(f_db, line))
    {
        stringstream getVals(line);
        string k;
        getVals>>k;
        if(k != key)
            newdb<<line<<endl;
        else
            found = true;
        
    }

    if(!found)
        cout<<"KEY NOT FOUND!"<<endl;

    newdb.close();
    f_db.close();
    remove("keydb.txt");
    rename("temp.txt", "keydb.txt");
    f_db.open("keydb.txt", ios::out | ios::app | ios::in);
}

int main()
{
    // WRITE PARSER
    
    // sample received string
    string s, key, method;

    Server sr;

    sr.handle_put("1", "100");
    sr.handle_delete("1");
    sr.handle_put("2", "100");
    sr.handle_delete("2");
    sr.handle_put("2", "200");
    sr.handle_put("4", "100");
    sr.handle_put("3", "100");
    // sr.handle_get("1");
    // sr.handle_get("3");
    sr.handle_delete("3");
    sr.handle_delete("4");

    return 0;
}
