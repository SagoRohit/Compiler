#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
using namespace std;

// symbolinfo

class SymbolInfo
{
private:
    string name;
    string type;
    SymbolInfo *next;

public:
    SymbolInfo(string name, string type)
    {
        this->name = name;
        this->type = type;
        this->next = nullptr;
    }

    string getName()
    {
        return this->name;
    }
    string getType()
    {
        return this->type;
    }
    SymbolInfo *getNext()
    {
        return next;
    }

    void setName(string name)
    {
        this->name = name;
    }
    void setType(string type)
    {
        this->type = type;
    }
    void setNext(SymbolInfo *next)
    {
        this->next = next;
    }
};

// scopetable

class ScopeTable
{
private:
    unsigned int bucketCount;
    ScopeTable *parentScope;
    SymbolInfo **table;
    int id;
    ofstream &output;

    unsigned int SDBMHash(string str, unsigned int num_buckets)
    {
        unsigned int hash = 0;
        unsigned int len = str.length();
        for (unsigned int i = 0; i < len; i++)
        {
            hash = ((str[i]) + (hash << 6) + (hash << 16) - hash) %
                   num_buckets;
        }
        return hash;
    }

public:
    ScopeTable(unsigned int buckets, int id, ScopeTable *parent, ofstream &out) : output(out)
    {
        this->id = id;
        this->bucketCount = buckets;
        this->parentScope = parent;

        table = new SymbolInfo *[bucketCount];
        for (unsigned int i = 0; i < bucketCount; i++)
        {
            table[i] = nullptr;
        }
    }

    bool insert(string name, string type)
    {
        unsigned int idx = SDBMHash(name, bucketCount);
        SymbolInfo *head = table[idx];
        int pos = 1;
        SymbolInfo *temp = head;
        while (temp != nullptr)
        {
            if (temp->getName() == name)
            {
                output << "\t\t\'" << name << "\'" << "already exists in the current ScopeTable" << endl;
                return false; // exists!!
            }
            temp = temp->getNext();
            pos++;
        }

        SymbolInfo *newSymbol = new SymbolInfo(name, type);
        newSymbol->setNext(head);
        table[idx] = newSymbol;
        output << "\t\tInserted in ScopeTable# " << id << " at position " << (idx + 1) << ", " << pos << endl;
        return true;
    }

    SymbolInfo *lookup(string name)
    {
        unsigned int idx = SDBMHash(name, bucketCount);
        SymbolInfo *temp = table[idx];

        while (temp != nullptr)
        {
            if (temp->getName() == name)
            {
                output << "\t\t\'" << name << "\'" << "found in ScopeTable# " << id << " at position " << idx + 1 << ", " << id << endl;

                return temp;
            }

            temp = temp->getNext();
        }
        return nullptr;
    }

    bool deleteSymbol(string name)
    {
        unsigned int idx = SDBMHash(name, bucketCount);
        SymbolInfo *curr = table[idx];
        SymbolInfo *prev = nullptr;

        while (curr != nullptr)
        {
            if (curr->getName() == name)
            {
                if (prev == nullptr)
                    table[idx] = curr->getNext();
                else
                    prev->setNext(curr->getNext());
                output << "\t\tDeleted " << "\'" << name << "\'" << " from ScopeTable# " << id << " at position " << idx + 1 << ", " << id << endl;

                delete curr;
                return true;
            }
            prev = curr;
            curr = curr->getNext();
        }
        output << "\t\tNot found in the current ScopeTable" << endl;
        return false;
    }

    // void print(int level=0)
    // {
    //     for (int i = 0; i < level; ++i) output << "\t";
    //     output << "\t\tScopeTable# " << id << endl;

    //     for (unsigned int i = 0; i < bucketCount; i++)
    //     {
    //         output << "\t\t"<<(i+1) << "-->";
    //         SymbolInfo *temp = table[i];
    //         while (temp != nullptr)
    //         {
    //             output << "<" << temp->getName() << "," << temp->getType() << ">" << endl;
    //             temp = temp->getNext();
    //         }
    //         output << endl;
    //     }
    // }

    string formatSymbolForPrint(const string &name, const string &type)
    {
        if (type.substr(0, 8) == "FUNCTION")
        {
            size_t firstSpace = type.find(' ', 9);
            string retType = type.substr(9, firstSpace - 9);
            string args = type.substr(firstSpace + 1);
            return "<" + name + ",FUNCTION," + retType + "<==(" + args + ")>>";
        }
        else if (type.substr(0, 6) == "STRUCT" || type.substr(0, 5) == "UNION")
        {
            size_t split = type.find(' ');
            string kind = type.substr(0, split);
            string rest = type.substr(split + 1);
            stringstream ss(rest);
            string dtype, dname;
            string formatted = "<" + name + "," + kind + ",{";
            bool first = true;
            while (ss >> dtype >> dname)
            {
                if (!first)
                    formatted += ",";
                formatted += "(" + dtype + "," + dname + ")";
                first = false;
            }
            formatted += "}>";
            return formatted;
        }
        else
        {
            return "<" + name + "," + type + ">";
        }
    }

    void print(int level = 1)
    {
        for (int i = 0; i < level; ++i)
            output << "\t";
        output << "ScopeTable# " << id << endl;

        for (unsigned int i = 0; i < bucketCount; ++i)
        {
            for (int j = 0; j < level; ++j)
                output << "\t";
            output << (i + 1) << "-->";
            SymbolInfo *temp = table[i];
            while (temp != nullptr)
            {
                output << formatSymbolForPrint(temp->getName(), temp->getType()) << " ";
                temp = temp->getNext();
            }
            output << endl;
        }
    }
    int getID()
    {
        return id;
    }

    ScopeTable *getParentScope()
    {
        return parentScope;
    }

    ~ScopeTable()
    {
        for (unsigned int i = 0; i < bucketCount; ++i)
        {
            SymbolInfo *curr = table[i];
            while (curr != nullptr)
            {
                SymbolInfo *next = curr->getNext();
                delete curr;
                curr = next;
            }
        }
        delete[] table;
    }
};

class SymbolTable
{
private:
    ScopeTable *currentScope;
    unsigned int bucketsize;
    int scopeCount;
    ofstream &output;

public:
    SymbolTable(unsigned int n, ofstream &out) : output(out)
    {
        bucketsize = n;
        scopeCount = 1;
        currentScope = new ScopeTable(bucketsize, scopeCount, nullptr, output);
    }

    void enterScope()
    {
        scopeCount++;
        ScopeTable *newScope = new ScopeTable(bucketsize, scopeCount, currentScope, output);
        output << "\t\tScopeTable# " << newScope->getID() << " created" << endl;
        currentScope = newScope;
    }

    bool exitScope()
    {
        if (currentScope->getParentScope() == nullptr)
            return false;
        ScopeTable *temp = currentScope;
        output << "\t\tScopeTable# " << currentScope->getID() << " removed" << endl;
        currentScope = currentScope->getParentScope();
        delete temp;
        return true;
    }
    void removeGlobalScope() {
        if (currentScope != nullptr) {
            output << "\tScopeTable# " << currentScope->getID() << " removed" << endl;
            delete currentScope;
            currentScope = nullptr;
        }
    }
    

    bool insert(string name, string type)
    {
        if (currentScope == nullptr)
            return false;
        return currentScope->insert(name, type);
    }

    bool remove(string name)
    {
        if (currentScope == nullptr)
            return false;
        return currentScope->deleteSymbol(name);
    }

    SymbolInfo *lookup(string name)
    {
        ScopeTable *temp = currentScope;
        while (temp != nullptr)
        {
            SymbolInfo *look = temp->lookup(name);
            if (look != nullptr)
                return look;
            temp = temp->getParentScope();
        }
        output << "\t\t\'" << name << "\'" << " not found in any of the ScopeTables" << endl;
        return nullptr;
    }

    void printCurrentScopeTable()
    {
        if (currentScope != nullptr)
            currentScope->print();
    }
    // void printAllScopeTable()
    // {
    //     ScopeTable *temp = currentScope;
    //     int level = 0;
    //     while (temp != nullptr)
    //     {
    //         temp->print(level);
    //         temp = temp->getParentScope();
    //         level++;
    //     }
    // }
    void printAllScopeTable()
{
    printScopeRecursive(currentScope, 0);
}

void printScopeRecursive(ScopeTable* scope, int level)
{
    if (scope == nullptr) return;

    // print parent first so the current scope comes last (like a stack trace)
    scope->print(level);  // child scope = less indented
    printScopeRecursive(scope->getParentScope(), level + 1);
}


    void endScope()
    {
        while (currentScope != nullptr)
        {
            output << "\tScopeTable# " << currentScope->getID() << " removed" << endl;
            ScopeTable *temp = currentScope;
            currentScope = currentScope->getParentScope();
            delete temp;
        }
    }

    ~SymbolTable()
    {
        while (currentScope != nullptr)
        {
            ScopeTable *temp = currentScope;
            currentScope = currentScope->getParentScope();
            delete temp;
        }
    }
};

int main()
{
    ifstream inputfile("sample_input.txt");
    ofstream outputfile("output.txt");

    if (!inputfile || !outputfile)
    {
        cout << "Error opening input or output file" << endl;
        return -1;
    }

    int bucketsize;
    inputfile >> bucketsize;
    inputfile.ignore();

    SymbolTable SymbolTable(bucketsize, outputfile);
    outputfile << "\tScopeTable# 1 created" << endl;

    string line;
    int cmdcount = 1;

    while (getline(inputfile, line))
    {
        if (line.empty())
            continue;

        outputfile << "Cmd " << cmdcount++ << ": " << line << endl;

        stringstream ss(line);
        string command;
        ss >> command;

        if (command == "I")
        {
            string name, type;
            ss >> name;
            if (!(ss >> type))
            {
                outputfile << "\tNumber of parameters mismatch for the command I" << endl;
                continue;
            }
            string rest;
            getline(ss, rest);
            if (!rest.empty())
            {
                type += rest;
            }
            SymbolTable.insert(name, type);
        }

        else if (command == "L")
        {
            string name, extra;
            ss >> name;
            if (ss >> extra)
            {
                outputfile << "\tNumber of parameters mismatch for the command L" << endl;
                continue;
            }
            SymbolTable.lookup(name);
        }

        else if (command == "D")
        {
            string name, extra;
            if (!(ss >> name))
            {
                outputfile << "\tNumber of parameters mismatch for the command D" << endl;
                continue;
            }
            if (ss >> extra)
            {
                outputfile << "\tNumber of parameters mismatch for the command D" << endl;
                continue;
            }
            SymbolTable.remove(name);
        }

        else if (command == "P")
        {
            string print;
            ss >> print;
            if (print == "A")
            {
                SymbolTable.printAllScopeTable();
            }
            else if (print == "C")
            {
                SymbolTable.printCurrentScopeTable();
            }
        }

        else if (command == "S")
        {
            SymbolTable.enterScope();
        }
        else if (command == "E")
        {
            if (!(SymbolTable.exitScope()))
            {
                outputfile << "\tCannot exit from global scope" << endl;
            }
        }
        else if (command == "Q") {
            while (SymbolTable.exitScope()); // remove all scopes except global
            SymbolTable.removeGlobalScope(); // now remove global
            break;
        }
        

        else
        {
            outputfile << "\tInvalid command" << endl;
        }
    }

    inputfile.close();
    outputfile.close();
}