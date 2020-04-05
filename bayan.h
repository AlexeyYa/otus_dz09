/*! @file bayan.h
 *  @brief удаление дупликатов файлов
 */

#ifndef BAYAN_H
#define BAYAN_H

#include "hash.h"

#include <boost/program_options.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <regex>

// Workaround for travis
#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif


/*!
 * @brief The FileData struct для хранения данных в BayanDataImpl
 */
struct FileData
{
    FileData(fs::path path, size_t size) : path(path), size(size){}
    fs::path path;
    size_t size;
};

bool operator<(const FileData& lhs, const FileData& rhs);

/*!
 * @brief The BayanData struct интерфейс для хранения данных и удаления дубликатов файлов
 */
struct BayanData
{
    virtual ~BayanData(){}
    virtual void Add(fs::path file) = 0;

    virtual void RemoveDuplicate() = 0;
};

/*!
 * @brief The BayanDataImpl struct реализация интерфейса считывающая все файлы и хранящая хэш для них
 */
template <typename Hash, size_t hash_size = Hash::size>
struct BayanDataImpl : BayanData
{
    BayanDataImpl(size_t buffer_size) : buffer_size(buffer_size){}
    virtual void Add(fs::path file) override;

    virtual void RemoveDuplicate() override;
private:
    Hash hash_func;
    size_t buffer_size;
    std::map<std::array<char, hash_size>, std::set<FileData>> m_data;
};

/*!
 * @brief The BayanDataImpl struct реализация интерфейса сравнивающая файлы по размеру и при совпадении сравнивающая хэш
 */
template <typename Hash, size_t hash_size = Hash::size>
struct BayanDataSizeFirstImpl : BayanData
{
    BayanDataSizeFirstImpl(size_t buffer_size) : buffer_size(buffer_size){}
    virtual void Add(fs::path file) override;

    virtual void RemoveDuplicate() override;
private:
    Hash hash_func;
    size_t buffer_size;
    std::map<size_t, std::set<fs::path>> m_data;
};

/*!
 *  @brief Класс для удаление дупликатов файлов
 */
class Bayan
{
public:
    Bayan();

    Bayan(const std::vector<std::string>& dirs, const std::vector<std::string>& excluded, size_t depth, size_t minsize,
          const std::vector<std::string>& masks, size_t block, const std::string& hashalg);

    void Run() const;
    void ProcessFolder(const fs::path& dir, const size_t current_depth) const;
    void ProcessFile(const fs::path& file) const;
private:
    bool m_isValid;
    bool m_haveMask;
    std::vector<fs::path> m_dirs;
    std::vector<fs::path> m_excluded;
    const size_t m_depth;
    const size_t m_minsize;
    std::regex m_masks;
    const size_t m_block;
    const std::string m_hashalg;
    std::unique_ptr<BayanData> m_filedata;
};

Bayan ParseArgs(int argc, char *argv[]);


#endif
