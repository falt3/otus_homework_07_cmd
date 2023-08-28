#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <ctime>
#include <functional>

//*****************************************************************
// Интерфейс команд
class ICommand {
public:
    virtual void execute(std::ostream &out) = 0;
    virtual ~ICommand() = default;
    virtual std::time_t getTime() = 0;
};

// Класс команды
class Command : public ICommand {
public:
    Command(std::string &name) { 
        m_name = name; 
        m_time = std::time(nullptr); 
    }
    ~Command() = default;
    void execute(std::ostream &out) override { out << " " << m_name; }
    std::time_t getTime() { return m_time; }
private:
    std::string m_name;
    std::time_t m_time;
};

// Класс блока команд
class BlockCommand: public ICommand {
public:
    BlockCommand() {}
    ~BlockCommand() = default;
    void execute(std::ostream &out) override {
        for (auto it = m_commands.begin(); it < m_commands.end(); ++it) {
            (*it)->execute(out);
            if (it != --m_commands.end())
                out << ",";
        }
    }
    int getCountCommands() { return m_commands.size(); }
    void addCommand(std::unique_ptr<ICommand> cmd) {
        m_commands.emplace_back(std::move(cmd));
    }
    std::time_t getTime() { 
        if (m_commands.size() > 0) 
            return m_commands[0]->getTime();
        else return 0;
    };
private:
    std::vector<std::unique_ptr<ICommand>> m_commands;
};

//*****************************************************************
// интерфейс интерпретатора
class IInterpret {  
public:
    virtual ~IInterpret() = default;
    virtual void execute() = 0;
    std::function<void(ICommand& cmd)> callBack_cmd; 
};

// Класс интерпретатора
class Interpret : public IInterpret {
public:
    Interpret(int count) : m_count(count) {}
    Interpret(int count, std::string fileName) : m_count(count), m_fileName(fileName) {}
    void execute() override;
protected:
    int m_count;
    std::string m_fileName;
private:
    void run(std::istream &in);
    int block(BlockCommand &cmd, std::istream &in);
    int dinamicBlock(BlockCommand &cmd, std::istream &in);
};

void Interpret::execute() 
{
    // выбираем откуда будем брать данные: из консоли или из файла
    if (m_fileName.size() == 0)
        run(std::cin);
    else {
        std::fstream fs(m_fileName, std::fstream::in);
        if (fs.is_open()) {
            run(fs);
            fs.close();
        }        
    }
}

void Interpret::run(std::istream &in)
{
    int res = 0;
    while (true) {
        BlockCommand cmd;
        if (res == 0) 
            res = block(cmd, in);
        else if (res == 1) // обычный блок закончился началом динамического "{"
            res = dinamicBlock(cmd, in);

        if (res >= 0) {
            callBack_cmd(cmd);
            if (res == 2)
                break;
        }
        else {
            break;
        }
    }
}

int Interpret::block(BlockCommand &cmd, std::istream &in) 
{
    std::string line;   
    while (std::getline(in, line) && !line.empty()) {
        if (line == "{") {
            if (cmd.getCountCommands() == 0) //
                return dinamicBlock(cmd, in);
            else {
                return 1;   // обработка блока и начало нового динамического
            }
        }
        else if (line == "}") { // ошибка ввода
            if (cmd.getCountCommands() == 0) return -1;    
            else return 2;  // если уже есть команды, обрабатываем их и выходим
        }
        else {
            cmd.addCommand(std::unique_ptr<ICommand>(new Command(line)));
            if (m_count == cmd.getCountCommands())
                return 0;
        }   
    }
    if (cmd.getCountCommands() == 0) return -1;
    else return 0;
}

int Interpret::dinamicBlock(BlockCommand &cmd, std::istream &in) 
{
    std::string line;    
    while (std::getline(in, line) && !line.empty()) {
        if (line == "{") {
            BlockCommand *cmd2 = new BlockCommand();
            dinamicBlock(*cmd2, in);
            if (cmd2->getCountCommands() > 0)
                cmd.addCommand(std::unique_ptr<ICommand>(cmd2));
        }
        else if (line == "}") {
            return 0;
        }
        else {
            cmd.addCommand(std::unique_ptr<ICommand>(new Command(line)));
        }
    }
    return -1;
}

//*****************************************************************
// класс логирования
class Log {
public:
    Log() {};
    void msg(std::time_t time, const std::string &msg);
};

void Log::msg(std::time_t time, const std::string &msg) 
{
    std::ostringstream fileName;
    fileName << "./bulk" << time << ".log";
    std::fstream fs(fileName.str(), std::fstream::app);
    if (fs.is_open()) {
        fs << msg << "\n";
        fs.close();
    }
}

//*****************************************************************

int main(int argc, const char *argv[]) 
{
    int sizeBlock = 3;
    if (argc > 1)
        sizeBlock = std::stoi(argv[1]);
    if (sizeBlock < 1) sizeBlock = 1;

    Log log;

    auto ff = [&log](ICommand& cmd) { 
        std::ostringstream strOut;
        strOut << "bulk:";
        cmd.execute(strOut);        
        std::cout << strOut.str() << std::endl;    
        log.msg(cmd.getTime(), strOut.str());                     
    };

    // Interpret interpret(sizeBlock, "./test_files/commands.txt");
    Interpret interpret(sizeBlock);
    interpret.callBack_cmd = ff;
    interpret.execute();

    return 0;
}