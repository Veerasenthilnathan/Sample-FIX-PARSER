
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <sstream>

// A simple structure to represent a parsed FIX Message
struct FixMessage {
    std::string beginString;                               // Tag 8
    std::string msgType;                                   // Tag 35
    std::string senderCompID;                              // Tag 49
    std::string targetCompID;                              // Tag 56
    std::unordered_map<int, std::string> fields;           // All other tags stored for quick O(1) lookup
};

class FixParser {
public:
    // Parses a raw FIX string using a specified delimiter character (usually SOH / \x01, or '|' for testing)
    static FixMessage parse(std::string_view rawMessage, char delimiter = '|') {
        FixMessage msg;
        size_t start = 0;
        size_t end = rawMessage.find(delimiter);

        while (end != std::string_view::npos) {
            std::string_view token = rawMessage.substr(start, end - start);
            if (!token.empty()) {
                processToken(token, msg);
            }
            start = end + 1;
            end = rawMessage.find(delimiter, start);
        }
        
        // Process the final token if there is no trailing delimiter
        if (start < rawMessage.size()) {
            processToken(rawMessage.substr(start), msg);
        }

        return msg;
    }

private:
    static void processToken(std::string_view token, FixMessage& msg) {
        size_t eqPos = token.find('=');
        if (eqPos == std::string_view::npos) return;

        std::string tagStr(token.substr(0, eqPos));
        std::string valStr(token.substr(eqPos + 1));
        
        try {
            int tag = std::stoi(tagStr);
            
            // Extract standard header fields directly for performance, store others in the map
            switch (tag) {
                case 8:  msg.beginString = valStr;  break;
                case 35: msg.msgType = valStr;      break;
                case 49: msg.senderCompID = valStr; break;
                case 56: msg.targetCompID = valStr; break;
                default:
                    msg.fields[tag] = valStr;
                    break;
            }
        } catch (...) {
            // Handle or log malformed tags gracefully in a production environment
        }
    }
};

int main() {
    // Standard New Order Single (35=D) test message using '|' as a visual delimiter
    std::string sampleFixOrder = "8=FIX.4.4|35=D|49=CLIENT_A|56=BROKER_B|11=ORDER_1001|55=BHP|54=1|38=500|44=43.50|";

    std::cout << "Parsing Sample FIX Message...\n\n";
    FixMessage parsedMsg = FixParser::parse(sampleFixOrder);

    // Display Header Fields
    std::cout << "[Header Information]\n";
    std::cout << "FIX Version (8): " << parsedMsg.beginString << "\n";
    std::cout << "Msg Type    (35): " << parsedMsg.msgType << " (New Order Single)\n";
    std::cout << "Sender ID   (49): " << parsedMsg.senderCompID << "\n";
    std::cout << "Target ID   (56): " << parsedMsg.targetCompID << "\n\n";

    // Display Body Fields
    std::cout << "[Order Body Fields]\n";
    if (parsedMsg.fields.count(11)) std::cout << "ClOrdID      (11): " << parsedMsg.fields[11] << "\n";
    if (parsedMsg.fields.count(55)) std::cout << "Symbol       (55): " << parsedMsg.fields[55] << "\n";
    if (parsedMsg.fields.count(54)) std::cout << "Side [1=Buy] (54): " << parsedMsg.fields[54] << "\n";
    if (parsedMsg.fields.count(38)) std::cout << "Quantity     (38): " << parsedMsg.fields[38] << "\n";
    if (parsedMsg.fields.count(44)) std::cout << "Price        (44): " << parsedMsg.fields[44] << "\n";

    return 0;
}
