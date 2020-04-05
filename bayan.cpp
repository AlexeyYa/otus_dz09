/*! @file bayan.h
 *  @brief удаление дупликатов файлов
 */

#include "bayan.h"

// Solution for string vector default value
// from https://stackoverflow.com/questions/40754810/default-value-for-vector-valued-boostprogram-options?rq=1
namespace std
{
    /*!
     * @brief operator << Вспомогательный оператор вывода в поток для поддержки значений по умолчанию для векторов строк
     * @param os Поток для вывода
     * @param vec Вектор строк
     * @return Ссылка на поток для дальнейшего вывода
     */
    std::ostream& operator<<(std::ostream &os, const std::vector<std::string> &vec)
    {
        for (const
             auto& item : vec)
        {
            os << item << " ";
        }
        return os;
    }
}

/*!
 * @brief operator < Вспомогательный оператор сравнения для BayanDataImpl
 * @param lhs Левый аргумент
 * @param rhs Правый аргумент
 * @return результат сравнения
 */
bool operator<(const FileData& lhs, const FileData& rhs)
{
    if (lhs.size != rhs.size)
        return lhs.size < rhs.size;
    return lhs.path < rhs.path;
}

/*!
 * @brief ParseArgs Обработчик аргументов командной строки
 * @param argc Количество аргументов командной строки
 * @param argv Аргументы командной строки
 * @return Объект для поиска и удаления дубликатов
 */
Bayan ParseArgs(int argc, char *argv[])
{
    using namespace boost::program_options;
    using vecst = std::vector<std::string>;

    const size_t default_readblock = 4096;

    try {
        options_description desc{"Options"};
        desc.add_options()
              ("help", "Help screen")
              ("dir,d", value<vecst>()->multitoken(), "Scan directories")
              ("exclude,e", value<vecst>()->multitoken()->default_value(vecst{}), "Excluded directories")
              ("depth", value<size_t>()->default_value(0), "Depth")
              ("minsize,s", value<size_t>()->default_value(1), "Minimal filesize")
              ("mask,m", value<vecst>()->multitoken()->default_value(vecst{}), "Allowed file masks")
              ("block,b", value<size_t>()->default_value(default_readblock), "Reading block size, bytes")
              ("hashalg,h", value<std::string>()->default_value("crc32"), "Hashing algorithm, available options: crc32");

        variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);
        notify(vm);

        if (vm.count("help"))
            std::cout << desc << '\n';
        else if (vm.count("dir"))
        {
            return Bayan{ vm["dir"].as<vecst>(),
                          vm["exclude"].as<vecst>(),
                          vm["depth"].as<size_t>(),
                          vm["minsize"].as<size_t>(),
                          vm["mask"].as<vecst>(),
                          vm["block"].as<size_t>(),
                          vm["hashalg"].as<std::string>()};
        }

    }
    catch (const error &ex)
    {
        std::cerr << "Invalid input, error: " << ex.what() << '\n';
    }
    return Bayan{};
}

/*!
 * @brief Bayan::Bayan Конструктор по умолчанию возвращающий невалидное состояние
 */
Bayan::Bayan() : m_depth(0), m_minsize(0), m_block(0)
{
    m_isValid = false;
}

/*!
 * @brief Bayan::Bayan Конструктор со всеми аргументами
 * @param dirs Директории для поиска
 * @param excluded Исключенные из поиска директории
 * @param depth Глубина поиска
 * @param minsize Минимальный размер файла
 * @param masks Маски файлов разрешенных для поиска дубликатов
 * @param block Размер блока для считывания
 * @param hashalg Алгоритм хэширования
 */
Bayan::Bayan(const std::vector<std::string>& dirs, const std::vector<std::string>& excluded, size_t depth, size_t minsize,
             const std::vector<std::string>& masks, size_t block, const std::string& hashalg)
           : m_depth(depth), m_minsize(minsize), m_block(block), m_hashalg(hashalg)
{
    m_isValid = true;
    m_haveMask = false;
    if (hashalg == "crc32")
    {
        m_filedata = std::make_unique<BayanDataSizeFirstImpl<CRC32HASH>>(m_block);
    }
    else if (hashalg == "md5")
    {
        m_filedata = std::make_unique<BayanDataSizeFirstImpl<MD5HASH>>(m_block);
    }
    else if (hashalg == "sha1")
    {
        m_filedata = std::make_unique<BayanDataSizeFirstImpl<SHA1HASH>>(m_block);
    }
    else
    {
        m_isValid = false;
        std::cerr << "Hasher " << hashalg <<
                     " is not supported. Use bayan -h to see available hashing algorithms" << std::endl;
        return;
    }

    if (m_block == 0)
    {
        m_isValid = false;
        std::cerr << "Can't have 0 block size" << std::endl;
        return;
    }

    if (!masks.empty())
    {
        m_haveMask = true;
        std::stringstream ss;
        bool first = true;
        for (auto mask : masks)
        {
            if (first)
                first = false;
            else
                ss << '|';
            ss << mask;
        }
        m_masks = std::regex(ss.str());
    }

    for (const auto& path : dirs)
    {
        if (fs::exists(path) && fs::is_directory(path))
        {
            m_dirs.emplace_back(path);
        }
        else
        {
            std::cerr << path << " is not a valid directory, aborting" << std::endl;
            m_isValid = false;
            return;
        }
    }

    for (const auto& path : excluded)
    {
        if (fs::exists(path) && fs::is_directory(path))
        {
            m_excluded.emplace_back(path);
        }
        else
        {
            std::cerr << path << " is not a valid directory, aborting" << std::endl;
            m_isValid = false;
            return;
        }
    }
}

/*!
 * @brief Bayan::Run Запуск обработки
 */
void Bayan::Run() const
{
    if (m_isValid)
    {
        for (const auto& dir : m_dirs)
        {
            std::cout << dir << std::endl;

            ProcessFolder(dir, 0);
        }
        m_filedata->RemoveDuplicate();
    }
    else
    {
        std::cerr << "Invalid arguments, can't continue" << std::endl;
    }
}

/*!
 * @brief Bayan::ProcessFolder Обработка директорий
 * @param dir Директория для поиска
 * @param current_depth Текущая глубина поиска
 */
void Bayan::ProcessFolder(const fs::path& dir, const size_t current_depth) const
{
    if (current_depth <= m_depth)
    {
        for (const auto& entry : fs::directory_iterator(dir))
        {
            // Обработка вложенных директорий
            if (fs::is_directory(entry.status()))
            {
                if (std::find(m_excluded.begin(), m_excluded.end(), entry) == m_excluded.end())
                {
                    ProcessFolder(entry, current_depth + 1);
                }
            }
            // Обработка файлов
            else if (fs::is_regular_file(entry.status()) && fs::file_size(entry) >= m_minsize)
            {
                if (!m_haveMask)
                    m_filedata->Add(entry);
                else
                {
                    std::smatch base_match;
                    if (std::regex_search(entry.path().filename().string(), m_masks))
                    {
                        m_filedata->Add(entry);
                    }
                }
            }
        }
    }
}

/*!
 * @brief BayanDataImpl<Hash, hash_size>::RemoveDuplicate Удаление дубликатов из списка файлов
 */
template <typename Hash, size_t hash_size>
void BayanDataImpl<Hash, hash_size>::RemoveDuplicate()
{
    for (const auto& hashcode : m_data)
    {
        auto cmpfile = *hashcode.second.begin();
        bool first = true;
        for (const auto& file : hashcode.second)
        {
            if (first)
            {
                first = false;
                continue;
            }
            // Duplicate removal part, same hash and size, or change to next
            if (cmpfile.size == file.size)
            {
                fs::remove(file.path);
                std::cout << "File removed: " << file.path << std::endl;
            }
            else
                cmpfile = file;
        }
    }
}

/*!
 * @brief BayanDataImpl<Hash, hash_size>::Add Добавление в список файлов для обработки
 * @param file
 */
template <typename Hash, size_t hash_size>
void BayanDataImpl<Hash, hash_size>::Add(fs::path file)
{
    auto tmphash = hash_func(file.string(), buffer_size);

    m_data[tmphash].emplace(file, fs::file_size(file));
}

/*!
 * @brief BayanDataSizeFirstImpl<Hash, hash_size>::RemoveDuplicate Удаление дубликатов из списка файлов
 */
template <typename Hash, size_t hash_size>
void BayanDataSizeFirstImpl<Hash, hash_size>::RemoveDuplicate()
{
    for (const auto& filesize : m_data)
    {
        if (filesize.second.size() > 1)
        {
            std::map<std::array<char, hash_size>, std::vector<fs::path>> hashes;
            for (const auto& file : filesize.second)
            {
                auto tmphash = hash_func(file.string(), buffer_size);
                hashes[tmphash].emplace_back(file);
            }

            for (const auto& hash : hashes)
            {

                auto cmpfile = *hash.second.begin();
                bool first = true;
                for (const auto& file : hash.second)
                {
                    if (first)
                    {
                        first = false;
                        continue;
                    }
                    fs::remove(file);
                    std::cout << "File removed: " << file.string() << std::endl;
                }
            }
        }
    }
}

/*!
 * @brief BayanDataSizeFirstImpl<Hash, hash_size>::Add Добавление в список файлов для обработки
 * @param file
 */
template <typename Hash, size_t hash_size>
void BayanDataSizeFirstImpl<Hash, hash_size>::Add(fs::path file)
{
    m_data[fs::file_size(file)].emplace(file);
}
