#ifndef CGI_PROCESS_HPP
#define CGI_PROCESS_HPP

#include <string>
#include <vector>
#include <map>

class CgiProcess
{
public:
    CgiProcess();
    ~CgiProcess();

    // Set the script path and interpreter
    void setScript(const std::string& scriptPath);
    void setInterpreter(const std::string& interpreterPath);

    // Set environment variables
    void setEnvironment(const std::map<std::string, std::string>& env);
    void addEnvironmentVariable(const std::string& key, const std::string& value);

    // Set POST body data to be sent to stdin
    void setInputData(const std::string& data);
    void setInputFd(int fd);

    // Execute the CGI script and return the output
    std::string execute();

    // Get execution status
    bool hasError() const;
    std::string getErrorMessage() const;
    int getExitStatus() const;

private:
    std::string _scriptPath;
    std::string _interpreterPath;
    std::map<std::string, std::string> _environment;
    std::string _inputData;
    int _inputFd;
    std::string _output;
    std::string _errorMessage;
    int _exitStatus;
    bool _hasError;

    // Helper methods
    char** createEnvironmentArray();
    void freeEnvironmentArray(char** env);
    char** createArgvArray();
    void freeArgvArray(char** argv);
    void handleChildProcess(int inputPipe[2], int outputPipe[2]);
    void handleChildProcessWithFd(int outputPipe[2], int inputFd);
    std::string readFromPipe(int fd);
};

#endif
