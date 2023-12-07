#include "CipherFactory.hpp"
#include "CipherMode.hpp"
#include "CipherType.hpp"
#include "ProcessCommandLine.hpp"
#include "TransformChar.hpp"
#include "ExceptionClass.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <thread>

int main(int argc, char* argv[])
{
    // Convert the command-line arguments into a more easily usable form
    const std::vector<std::string> cmdLineArgs{argv, argv + argc};

    // Options that might be set by the command-line arguments
    ProgramSettings settings{false, false, "", "", {}, {}, CipherMode::Encrypt};

    try {
        processCommandLine(cmdLineArgs, settings);
    } catch ( const MissingArgument& e ) {
        std::cerr << "[error] Missing argument: " << e.what() << std::endl;
        return 1;
    } catch ( const UnknownArgument& e ) {
        std::cerr << "[error] Unknown argument: " << e.what() << std::endl;
        return 1;
    } catch ( const InvalidInput& e ) {
        std::cerr << "[error] Invalid input : " << e.what()<< std::endl;
    }

    // Handle help, if requested
    if (settings.helpRequested) {
        // Line splitting for readability
        std::cout
            << "Usage: mpags-cipher [-h/--help] [--version] [-i <file>] [-o <file>] [-c <cipher>] [-k <key>] [--encrypt/--decrypt]\n\n"
            << "Encrypts/Decrypts input alphanumeric text using classical ciphers\n\n"
            << "Available options:\n\n"
            << "  -h|--help        Print this help message and exit\n\n"
            << "  --version        Print version information\n\n"
            << "  -i FILE          Read text to be processed from FILE\n"
            << "                   Stdin will be used if not supplied\n\n"
            << "  -o FILE          Write processed text to FILE\n"
            << "                   Stdout will be used if not supplied\n\n"
            << "                   Stdout will be used if not supplied\n\n"
            << "  --multi-cipher N Specify the number of ciphers to be used in sequence\n"
            << "                   N should be a positive integer - defaults to 1"
            << "  -c CIPHER        Specify the cipher to be used to perform the encryption/decryption\n"
            << "                   CIPHER can be caesar, playfair, or vigenere - caesar is the default\n\n"
            << "  -k KEY           Specify the cipher KEY\n"
            << "                   A null key, i.e. no encryption, is used if not supplied\n\n"
            << "  --encrypt        Will use the cipher to encrypt the input text (default behaviour)\n\n"
            << "  --decrypt        Will use the cipher to decrypt the input text\n\n"
            << std::endl;
        // Help requires no further action, so return from main
        // with 0 used to indicate success
        return 0;
    }

    // Handle version, if requested
    // Like help, requires no further action,
    // so return from main with zero to indicate success
    if (settings.versionRequested) {
        std::cout << "0.5.0" << std::endl;
        return 0;
    }

    // Initialise variables
    char inputChar{'x'};
    std::string cipherText;

    // Read in user input from stdin/file
    if (!settings.inputFile.empty()) {
        // Open the file and check that we can read from it
        std::ifstream inputStream{settings.inputFile};
        if (!inputStream.good()) {
            std::cerr << "[error] failed to create istream on file '"
                      << settings.inputFile << "'" << std::endl;
            return 1;
        }

        // Loop over each character from the file
        while (inputStream >> inputChar) {
            cipherText += transformChar(inputChar);
        }

    } else {
        // Loop over each character from user input
        // (until Return then CTRL-D (EOF) pressed)
        while (std::cin >> inputChar) {
            cipherText += transformChar(inputChar);
        }
    }

    /*
    3. Loop over the number of threads you want to use (should be configurable but don’t worry about that now!)
    4. For each iteration, take the next chunk from the input string
    5. Start a new thread to run a lambda function that calls the ‘applyCipher’ function on the constructed Cipher object
    6. Loop over the futures and wait until they are all completed
    7. Get the results from them and assemble the final string
    */

    // Request construction of the appropriate cipher(s)
    std::vector<std::unique_ptr<Cipher>> ciphers;
    std::size_t nCiphers{settings.cipherType.size()};
    ciphers.reserve(nCiphers);
    for (std::size_t iCipher{0}; iCipher < nCiphers; ++iCipher) {
        try{
            ciphers.push_back(CipherFactory::makeCipher(
                settings.cipherType[iCipher], settings.cipherKey[iCipher]));
        } catch(const InvalidInput& w) {
            std::cerr << "[warning] Invalid key : " << w.what() << std::endl;
            return 1;
        }
        // Check that the cipher was constructed successfully
        if (!ciphers.back()) {
            std::cerr << "[error] problem constructing requested cipher"
                      << std::endl;
            return 1;
        }
    }

    // If we are decrypting, we need to reverse the order of application of the ciphers
    if (settings.cipherMode == CipherMode::Decrypt) {
        std::reverse(ciphers.begin(), ciphers.end());
    }

    std::size_t numthreads{2};
    std::size_t middle{(cipherText.size()+1)/2};

    std::vector<std::string> stringsegments={cipherText.substr(0,middle),cipherText.substr(middle)};
    std::vector< std::future< std::string > > futures;

    // Run the cipher(s) on the input text, specifying whether to encrypt/decrypt
    for (const auto& cipher : ciphers) {
        //cipherText = cipher->applyCipher(cipherText, settings.cipherMode);
        for(std::size_t n{0};n<numthreads;n++){ //loop over number of threads
            //std::string substring{};//put chunk infomation into the substr() argument
            std::string substringsection{stringsegments[n]};
            std::cout << n << " " << substringsection << std::endl;
            //create a new thread to run a lambda function which calls the applyCipher function on the constructed cipher object
            auto fn = [&](const auto ciphercopy){
                substringsection = ciphercopy->applyCipher(substringsection,settings.cipherMode);
                //causes problems -- says that cipher is deleted when using & and cannot use std::move as says the same thing
                //have not found a way to move unique pointer into lambda but no errors are appearing in vs code, only on compile
                //not sure what to do????
                return substringsection;};
            futures.push_back(std::async(std::launch::async,fn,cipher));  
            //loop over all futures and wait until all completed (i.e. loop over vector and do .join()?)
            //get results and assemble the final string
        }
    }

    std::string intermediate{""};

    for (std::size_t j{0};j<futures.size();j++){
        std::future_status status{std::future_status::ready};
        status = futures[j].wait_for(std::chrono::seconds(1));
        if (status == std::future_status::ready) {
            intermediate += futures[j].get();
        }
    }

    cipherText = intermediate;

    // Output the encrypted/decrypted text to stdout/file
    if (!settings.outputFile.empty()) {
        // Open the file and check that we can write to it
        std::ofstream outputStream{settings.outputFile};
        if (!outputStream.good()) {
            std::cerr << "[error] failed to create ostream on file '"
                      << settings.outputFile << "'" << std::endl;
            return 1;
        }

        // Print the encrypted/decrypted text to the file
        outputStream << cipherText << std::endl;

    } else {
        // Print the encrypted/decrypted text to the screen
        std::cout << cipherText << std::endl;
    }

    // No requirement to return from main, but we do so for clarity
    // and for consistency with other functions
    
    return 0;
}
