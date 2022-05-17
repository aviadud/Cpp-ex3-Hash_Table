/**
 * @file SpamDetector.cpp
 * @author Aviad Dudkevich
 * @brief Determine if a file is spam or not based on CSV file and given threshold.
 */
#include <iostream>
#include <fstream>
#include <regex>
#include <set>
#include "HashMap.hpp"


// Constants
static const int NUMBER_OF_ARGUMENTS = 4;
static const int DATABASE_PATH = 1;
static const int MASSAGE_PATH = 2;
static const int THRESHOLD = 3;
const std::regex VALID_LINE("[^,]+,[0-9]+[\n\r]?");
static const char *WRONG_USAGE_MSG = "Usage: SpamDetector <database path> <message path> "
                                     "<threshold>\n";
static const char *INVALID_INPUT_MSG = "Invalid input\n";
static const char *MEMORY_MSG_ERROR = "Memory error occurred\n";
static const char *OVER_THRESHOLD_MSG = "SPAM";
static const char *UNDER_THRESHOLD_MSG = "NOT_SPAM";
static const char COMMA = ',';

using std::string;
using std::vector;
using std::set;

/**
 * a class to represent input file error.
 */
class InvalidInput : public std::exception
{
public:
    const char *what() const noexcept override
    {
        return INVALID_INPUT_MSG;
    }
};

/**
 * Take a string and making every upper case letter to low case.
 * @param str reference to a string.
 * @return reference to the string.
 */
string &makeStringLowerCase(string &str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return str;
}


/**
 * Create the HashMap from database file. Throws InvalidInput if the file invalid.
 * @param databaseFile reference to ifstream.
 * @param databaseMap reference to HashMap.
 * @param wordsLen reference to set of size_t - to keep track of all possible words length.
 */
void createDatabaseMap(std::ifstream &databaseFile, HashMap<string, int> &databaseMap,
                       set<size_t> &wordsLen)
{
    string line, sequence;
    string::size_type commaIndex;
    int score;
    while (databaseFile.good())
    {
        std::getline(databaseFile, line);
        if (!std::regex_match(line, VALID_LINE))
        {
            if (line.empty() && databaseFile.eof())
            {
                break;
            }
            throw InvalidInput();
        }
        commaIndex = line.find(COMMA);
        sequence = line.substr(0, commaIndex);
        makeStringLowerCase(sequence);
        score = std::stoi(line.substr(commaIndex + 1));
        if (score < 0)
        {
            throw InvalidInput();
        }
        databaseMap[sequence] = score;
        wordsLen.emplace(sequence.size());
    }
    if (databaseFile.fail() && !databaseFile.eof())
    {
        throw InvalidInput();
    }
}

/**
 * Calculate the score to the given massage file based on databaseMap. To avoid missing any
 * possible sequence - I used brute force. For every size of possible sequence in database -
 * search all the input massage with all possible frames of that size.
 * This function can throw bad_alloc exception.
 * @param massageFile reference to ifstream.
 * @param databaseMap reference to HashMap.
 * @param wordsLen reference to set of size_t.
 * @return the score the massage gets based on database.
 */
int generateScore(std::ifstream &massageFile, HashMap<string, int> &databaseMap,
                  set<size_t> &wordsLen)
{

    int result = 0;
    if (!wordsLen.empty())
    {
        char *currentSequence = new char[*wordsLen.rbegin()]; // can throw bad_alloc
        string currentString;
        std::streampos cursor = massageFile.tellg();
        for (size_t currentLen: wordsLen)
        {
            while (massageFile.readsome(currentSequence, currentLen) ==
                   static_cast<std::streamsize> (currentLen)) //check frame
            {
                currentString = string(currentSequence, currentLen);
                makeStringLowerCase(currentString);
                if (databaseMap.containsKey(currentString))
                {
                    result += databaseMap[currentString];
                }
                massageFile.seekg(cursor);
                massageFile.seekg(1, std::ios::cur); // go back and increase cursor by 1.
                cursor = massageFile.tellg();
            }
            if (massageFile.fail() && !massageFile.eof())
            {
                throw InvalidInput();
            }
            massageFile.seekg(std::ios::beg); // go to the beginning of the file.
            cursor = massageFile.tellg();
        }
        delete[] currentSequence;
    }
    return result;
}

/**
 * This program gets 2 files, database and massage, and number, for threshold, and print "SPAM" or
 * "NOT_SPAM" if the message is spam or not. This is based on the sequences given in the database
 * file and their score. For every sequence that appear in the message file - add the score to the
 * total score. If the total score is higher then threshold - prints "SPAM", prints "NOT_SPAM"
 * otherwise.
 * @param argc argument counter.
 * @param argv arguments vector.
 * @return 0 if successful, 1 otherwise.
 */
int main(int argc, char *argv[])
{
    if (argc != NUMBER_OF_ARGUMENTS)
    {
        std::cerr << WRONG_USAGE_MSG;
        return EXIT_FAILURE;
    }
    try
    {
        const double threshold = std::stod(argv[THRESHOLD]);
        if (threshold <= 0)
        {
            throw InvalidInput();
        }
        std::ifstream databaseFile(argv[DATABASE_PATH]), massageFile(argv[MASSAGE_PATH]);
        HashMap<string, int> databaseMap;
        set<size_t> wordsLen;
        createDatabaseMap(databaseFile, databaseMap, wordsLen);
        if (generateScore(massageFile, databaseMap, wordsLen) >= threshold)
        {
            std::cout << OVER_THRESHOLD_MSG << std::endl;
        }
        else
        {
            std::cout << UNDER_THRESHOLD_MSG << std::endl;
        }
    }
    catch (InvalidInput &ex)
    {
        std::cerr << INVALID_INPUT_MSG;
        return EXIT_FAILURE;
    }
    catch (std::bad_alloc &ex)
    {
        std::cerr << MEMORY_MSG_ERROR;
        return EXIT_FAILURE;
    }
    catch (std::invalid_argument &ex) // if std::stod fail.
    {
        std::cerr << INVALID_INPUT_MSG;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
