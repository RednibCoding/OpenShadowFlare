#include "libs/RKC_RPG_SCRIPT/rkc_rpg_script.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <utility>

namespace osf::script {
namespace {

constexpr std::array<std::uint8_t, 12> kHeader = {
    'S', 'c', 'e', 'n', 'a', 'S',
    'c', 'r', 'i', 'p', 't', 'V',
};
constexpr std::uint32_t kMaximumItemCount = 1'000'000;
constexpr std::uint32_t kMaximumStringSize = 16 * 1024 * 1024;

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

class Reader {
public:
    explicit Reader(const std::vector<std::uint8_t>& bytes)
        : bytes_(bytes) {}

    bool readI32(std::int32_t& value) {
        if (remaining() < 4) {
            return false;
        }
        const std::uint32_t raw =
            static_cast<std::uint32_t>(bytes_[offset_]) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 1]) << 8u) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 2]) << 16u) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 3]) << 24u);
        value = raw <= static_cast<std::uint32_t>(
                           std::numeric_limits<std::int32_t>::max())
                    ? static_cast<std::int32_t>(raw)
                    : -static_cast<std::int32_t>(~raw) - 1;
        offset_ += 4;
        return true;
    }

    bool readBytes(std::size_t size, std::string& value) {
        if (size > remaining()) {
            return false;
        }
        value.assign(
            bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
            bytes_.begin() +
                static_cast<std::ptrdiff_t>(offset_ + size));
        offset_ += size;
        return true;
    }

    bool skip(std::size_t size) {
        if (size > remaining()) {
            return false;
        }
        offset_ += size;
        return true;
    }

    std::size_t offset() const {
        return offset_;
    }

    std::size_t remaining() const {
        return bytes_.size() - offset_;
    }

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t offset_ = 0;
};

bool readCount(
    Reader& input,
    std::uint32_t& count,
    std::size_t minimum_item_size) {
    std::int32_t signed_count = 0;
    if (!input.readI32(signed_count) || signed_count < 0) {
        return false;
    }
    count = static_cast<std::uint32_t>(signed_count);
    return count <= kMaximumItemCount &&
           (minimum_item_size == 0 ||
            static_cast<std::size_t>(count) <=
                input.remaining() / minimum_item_size);
}

bool readFlags(
    Reader& input,
    std::vector<FlagDefinition>& flags) {
    std::uint32_t count = 0;
    if (!readCount(input, count, 8)) {
        return false;
    }
    flags.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index) {
        FlagDefinition flag;
        if (!input.readI32(flag.id) ||
            !input.readI32(flag.initial_value)) {
            return false;
        }
        flags.push_back(flag);
    }
    return true;
}

}  // namespace

bool ScriptData::load(
    const std::filesystem::path& path,
    std::string* error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        setError(
            error,
            "The scenario script could not be opened: " +
                path.string());
        clear();
        return false;
    }
    const std::vector<std::uint8_t> bytes{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()};
    if (!file.eof() && file.fail()) {
        setError(
            error,
            "The scenario script could not be read: " +
                path.string());
        clear();
        return false;
    }
    return decode(bytes, error);
}

bool ScriptData::decode(
    const std::vector<std::uint8_t>& bytes,
    std::string* error) {
    clear();
    if (bytes.size() < 16 ||
        !std::equal(kHeader.begin(), kHeader.end(), bytes.begin())) {
        setError(error, "The scenario script header is invalid.");
        return false;
    }

    Reader input(bytes);
    std::string full_header;
    if (!input.readBytes(16, full_header)) {
        return false;
    }
    version_.assign(full_header.begin() + 12, full_header.end());
    while (!version_.empty() && version_.back() == '\0') {
        version_.pop_back();
    }

    if (!readFlags(input, temporary_flags_) ||
        !readFlags(input, network_flags_)) {
        setError(error, "The scenario script flag table is invalid.");
        clear();
        return false;
    }

    std::uint32_t message_count = 0;
    if (!readCount(input, message_count, 8)) {
        setError(error, "The scenario script message table is invalid.");
        clear();
        return false;
    }
    messages_.reserve(message_count);
    for (std::uint32_t index = 0; index < message_count; ++index) {
        Message message;
        std::int32_t size = 0;
        if (!input.readI32(message.id) ||
            !input.readI32(size) ||
            size < 0 ||
            static_cast<std::uint32_t>(size) > kMaximumStringSize ||
            !input.readBytes(
                static_cast<std::size_t>(size), message.text)) {
            setError(error, "A scenario script message is invalid.");
            clear();
            return false;
        }
        for (char& byte : message.text) {
            byte = static_cast<char>(
                ~static_cast<unsigned char>(byte));
        }
        messages_.push_back(std::move(message));
    }

    std::uint32_t status_count = 0;
    if (!readCount(input, status_count, 16)) {
        setError(error, "The scenario script status table is invalid.");
        clear();
        return false;
    }
    statuses_.reserve(status_count);
    for (std::uint32_t index = 0; index < status_count; ++index) {
        std::int32_t networked = 0;
        Status status;
        if (!input.readI32(networked) ||
            !input.readI32(status.kind) ||
            !input.readI32(status.character_number) ||
            !input.readI32(status.sentence)) {
            setError(error, "A scenario script status is invalid.");
            clear();
            return false;
        }
        status.networked = networked != 0;
        statuses_.push_back(status);
    }

    std::uint32_t sentence_count = 0;
    if (!readCount(input, sentence_count, 4)) {
        setError(error, "The scenario sentence table is invalid.");
        clear();
        return false;
    }
    sentences_.reserve(sentence_count);
    for (std::uint32_t sentence_index = 0;
         sentence_index < sentence_count;
         ++sentence_index) {
        std::uint32_t command_count = 0;
        if (!readCount(input, command_count, 8)) {
            setError(error, "A scenario sentence is invalid.");
            clear();
            return false;
        }
        Sentence sentence;
        sentence.commands.reserve(command_count);
        for (std::uint32_t command_index = 0;
             command_index < command_count;
             ++command_index) {
            Command command;
            std::uint32_t operand_count = 0;
            if (!input.readI32(command.opcode) ||
                !readCount(input, operand_count, 8)) {
                setError(error, "A scenario command is invalid.");
                clear();
                return false;
            }
            command.operands.reserve(operand_count);
            for (std::uint32_t operand_index = 0;
                 operand_index < operand_count;
                 ++operand_index) {
                Operand operand;
                if (!input.readI32(operand.type) ||
                    !input.readI32(operand.value)) {
                    setError(error, "A scenario operand is invalid.");
                    clear();
                    return false;
                }
                command.operands.push_back(operand);
            }
            sentence.commands.push_back(std::move(command));
        }
        sentences_.push_back(std::move(sentence));
    }

    if (input.remaining() != 0) {
        setError(error, "The scenario script has trailing data.");
        clear();
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

void ScriptData::clear() {
    version_.clear();
    temporary_flags_.clear();
    network_flags_.clear();
    messages_.clear();
    statuses_.clear();
    sentences_.clear();
}

const std::string& ScriptData::version() const {
    return version_;
}

const std::vector<FlagDefinition>&
ScriptData::temporaryFlags() const {
    return temporary_flags_;
}

const std::vector<FlagDefinition>&
ScriptData::networkFlags() const {
    return network_flags_;
}

const std::vector<Message>& ScriptData::messages() const {
    return messages_;
}

const std::vector<Status>& ScriptData::statuses() const {
    return statuses_;
}

const std::vector<Sentence>& ScriptData::sentences() const {
    return sentences_;
}

const Message* ScriptData::findMessage(std::int32_t id) const {
    const auto found = std::find_if(
        messages_.begin(),
        messages_.end(),
        [id](const Message& message) {
            return message.id == id;
        });
    return found == messages_.end() ? nullptr : &*found;
}

const Status* ScriptData::findStatus(
    std::int32_t kind,
    std::int32_t character_number) const {
    const auto found = std::find_if(
        statuses_.begin(),
        statuses_.end(),
        [kind, character_number](const Status& status) {
            return status.kind == kind &&
                   status.character_number == character_number;
        });
    return found == statuses_.end() ? nullptr : &*found;
}

const Sentence* ScriptData::findSentence(
    std::int32_t index) const {
    if (index < 0 ||
        static_cast<std::size_t>(index) >= sentences_.size()) {
        return nullptr;
    }
    return &sentences_[static_cast<std::size_t>(index)];
}

}  // namespace osf::script
