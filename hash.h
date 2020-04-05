/*! @file hash.h
 *  @brief хэш функции для баяна
 */

#ifndef BAYANHASH_H
#define BAYANHASH_H

#include <boost/crc.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/uuid/detail/sha1.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <array>
#include <fstream>

/*!
 * @brief The CRC32HASH struct
 */
struct CRC32HASH
{
    const static size_t size = 4;

    // https://codereview.stackexchange.com/a/208592
    /*!
     * @brief operator () оператор хэширования
     * @param fp путь к файлу
     * @param buffer_size размер блока для чтения
     * @return массив байт с вычисленным кэшем
     */
    std::array<char,4> operator()(const std::string &fp, size_t buffer_size)
    {
        boost::crc_32_type crc;
        std::vector<char> buffer(buffer_size);
        std::ifstream stream(fp, std::ios::in|std::ios::binary);
        if(!stream) std::cerr << ("Could not open file for CRC calculation: " + fp) << std::endl;
        do {
            stream.read(&buffer[0], buffer_size);
            size_t byte_cnt = static_cast<size_t>(stream.gcount());
            crc.process_bytes(&buffer[0], byte_cnt);
        } while(stream);

        std::array<char, 4> result;
        for (size_t i = 0; i < 4; i++)
            result[i] = static_cast<char>(crc.checksum() >> (i * 8));
        return result;
    }
};

/*!
 * @brief The MD5HASH struct
 */
struct MD5HASH
{
    const static size_t size = 16;

    /*!
     * @brief operator () оператор хэширования
     * @param fp путь к файлу
     * @param buffer_size размер блока для чтения
     * @return массив байт с вычисленным кэшем
     */
    std::array<char,16> operator()(const std::string &fp, size_t buffer_size)
    {
        boost::uuids::detail::md5 md5;
        std::vector<char> buffer(buffer_size);
        std::ifstream stream(fp, std::ios::in|std::ios::binary);
        if(!stream) std::cerr << ("Could not open file for CRC calculation: " + fp) << std::endl;
        do {
            stream.read(&buffer[0], buffer_size);
            size_t byte_cnt = static_cast<size_t>(stream.gcount());
            md5.process_bytes(&buffer[0], byte_cnt);
        } while(stream);

        boost::uuids::detail::md5::digest_type digest;
        md5.get_digest(digest);
        auto ptr = reinterpret_cast<char*>(digest);

        std::array<char, 16> result;
        for (size_t i = 0; i < 16; i++)
            result[i] = *(ptr + i);
        return result;
    }
};

/*!
 * @brief The SHA1HASH struct
 */
struct SHA1HASH
{
    const static size_t size = 20;

    /*!
     * @brief operator () оператор хэширования
     * @param fp путь к файлу
     * @param buffer_size размер блока для чтения
     * @return массив байт с вычисленным кэшем
     */
    std::array<char,20> operator()(const std::string &fp, size_t buffer_size)
    {
        boost::uuids::detail::sha1 sha1;
        std::vector<char> buffer(buffer_size);
        std::ifstream stream(fp, std::ios::in|std::ios::binary);
        if(!stream) std::cerr << ("Could not open file for CRC calculation: " + fp) << std::endl;
        do {
            stream.read(&buffer[0], buffer_size);
            size_t byte_cnt = static_cast<size_t>(stream.gcount());
            sha1.process_bytes(&buffer[0], byte_cnt);
        } while(stream);

        boost::uuids::detail::sha1::digest_type digest;
        sha1.get_digest(digest);
        auto ptr = reinterpret_cast<char*>(digest);

        std::array<char, 20> result;
        for (size_t i = 0; i < 20; i++)
            result[i] = *(ptr + i);
        return result;
    }
};

#endif
