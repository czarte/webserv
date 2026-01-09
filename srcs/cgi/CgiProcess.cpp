#include "cgi/CgiProcess.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <cerrno>
#include <sstream>

CgiProcess::CgiProcess()
    : _exitStatus(-1), _hasError(false)
{
}

CgiProcess::~CgiProcess()
{
}

void CgiProcess::setScript(const std::string& scriptPath)
{
    _scriptPath = scriptPath;
}

void CgiProcess::setInterpreter(const std::string& interpreterPath)
{
    _interpreterPath = interpreterPath;
}

void CgiProcess::setEnvironment(const std::map<std::string, std::string>& env)
{
    _environment = env;
}

void CgiProcess::addEnvironmentVariable(const std::string& key, const std::string& value)
{
    _environment[key] = value;
}

void CgiProcess::setInputData(const std::string& data)
{
    _inputData = data;
}

std::string CgiProcess::execute()
{
    _output.clear();
    _hasError = false;
    _errorMessage.clear();

    // Create pipes for communication
    int inputPipe[2];  // For sending data to CGI script
    int outputPipe[2]; // For receiving data from CGI script

    if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1)
    {
        _hasError = true;
        _errorMessage = "Failed to create pipes: " + std::string(strerror(errno));
        return "";
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        _hasError = true;
        _errorMessage = "Failed to fork process: " + std::string(strerror(errno));
        close(inputPipe[0]);
        close(inputPipe[1]);
        close(outputPipe[0]);
        close(outputPipe[1]);
        return "";
    }

    if (pid == 0)  // Child process
    {
        handleChildProcess(inputPipe, outputPipe);
        // If we reach here, execve failed
        exit(1);
    }
    else  // Parent process
    {
        // Close unused pipe ends
        close(inputPipe[0]);
        close(outputPipe[1]);

        // Write input data to child's stdin if needed
        if (!_inputData.empty())
        {
            ssize_t written = write(inputPipe[1], _inputData.c_str(), _inputData.length());
            if (written == -1)
            {
                _hasError = true;
                _errorMessage = "Failed to write to child process: " + std::string(strerror(errno));
            }
        }
        close(inputPipe[1]);

        // Read output from child's stdout
        _output = readFromPipe(outputPipe[0]);
        close(outputPipe[0]);

        // Wait for child to complete
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            _exitStatus = WEXITSTATUS(status);
            if (_exitStatus != 0)
            {
                _hasError = true;
                _errorMessage = "CGI script exited with status: " + std::to_string(_exitStatus);
            }
        }
        else
        {
            _hasError = true;
            _errorMessage = "CGI script terminated abnormally";
        }
    }

    return _output;
}

void CgiProcess::handleChildProcess(int inputPipe[2], int outputPipe[2])
{
    // Redirect stdin to read from input pipe
    dup2(inputPipe[0], STDIN_FILENO);
    close(inputPipe[0]);
    close(inputPipe[1]);

    // Redirect stdout to write to output pipe
    dup2(outputPipe[1], STDOUT_FILENO);
    close(outputPipe[0]);
    close(outputPipe[1]);

    // Prepare environment and arguments
    char** env = createEnvironmentArray();
    char** argv = createArgvArray();

    // Execute the script
    execve(_interpreterPath.c_str(), argv, env);

    // If execve returns, it failed
    freeEnvironmentArray(env);
    freeArgvArray(argv);
}

char** CgiProcess::createEnvironmentArray()
{
    char** env = new char*[_environment.size() + 1];
    size_t i = 0;

    for (std::map<std::string, std::string>::const_iterator it = _environment.begin();
         it != _environment.end(); ++it)
    {
        std::string envVar = it->first + "=" + it->second;
        env[i] = new char[envVar.length() + 1];
        strcpy(env[i], envVar.c_str());
        ++i;
    }
    env[i] = NULL;

    return env;
}

void CgiProcess::freeEnvironmentArray(char** env)
{
    for (size_t i = 0; env[i] != NULL; ++i)
        delete[] env[i];
    delete[] env;
}

char** CgiProcess::createArgvArray()
{
    char** argv = new char*[3];
    argv[0] = new char[_interpreterPath.length() + 1];
    strcpy(argv[0], _interpreterPath.c_str());
    argv[1] = new char[_scriptPath.length() + 1];
    strcpy(argv[1], _scriptPath.c_str());
    argv[2] = NULL;
    return argv;
}

void CgiProcess::freeArgvArray(char** argv)
{
    for (size_t i = 0; argv[i] != NULL; ++i)
        delete[] argv[i];
    delete[] argv;
}

std::string CgiProcess::readFromPipe(int fd)
{
    std::string result;
    char buffer[4096];
    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
    {
        result.append(buffer, bytesRead);
    }

    return result;
}

bool CgiProcess::hasError() const
{
    return _hasError;
}

std::string CgiProcess::getErrorMessage() const
{
    return _errorMessage;
}

int CgiProcess::getExitStatus() const
{
    return _exitStatus;
}
