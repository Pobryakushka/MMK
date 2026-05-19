#ifndef NUMERIC_CONVERSIONS_H
#define NUMERIC_CONVERSIONS_H

#   if __cplusplus < 201103L

#   include <string>
#   include <sstream>

    namespace numeric_conversions {
        template <typename T>
        std::string to_string(T value) {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }

        template <typename T>
        T from_string(const std::string &str) {
            std::istringstream iss(str);
            T res;
            iss >> res;
            return res;
        }

        inline double stoi(const std::string &str) {
            return from_string<int>(str);
        }
        inline long stol(const std::string &str) {
            return from_string<long>(str);
        }
        inline long long stoll(const std::string &str) {
            return from_string<long long>(str);
        }

        inline unsigned long stoul(const std::string &str) {
            return from_string< unsigned long>(str);
        }
        inline unsigned long long stoull(const std::string &str) {
            return from_string<unsigned long long>(str);
        }

        inline float stof(const std::string &str) {
            return from_string<float>(str);
        }
        inline double stod(const std::string &str) {
            return from_string<double>(str);
        }
        inline long double stold(const std::string &str) {
            return from_string<long double>(str);
        }
    }
#   endif
#endif // NUMERIC_CONVERSIONS_H
