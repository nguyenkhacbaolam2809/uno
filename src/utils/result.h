#ifndef RESULT_H
#define RESULT_H

#include <optional>
#include <string>
#include <utility>

template <typename T>
class Result {
public:
    static Result ok(T val) { return Result(OkTag{}, std::move(val)); }
    static Result fail(std::string err) { return Result(FailTag{}, std::move(err)); }

    bool isOk() const { return m_value.has_value(); }
    bool isFail() const { return !m_value.has_value(); }

    T & value() { return *m_value; }
    const T & value() const { return *m_value; }

    std::string & error() { return m_error; }
    const std::string & error() const { return m_error; }

    T valueOr(T fallback) const { return m_value.value_or(fallback); }

private:
    struct OkTag {};
    struct FailTag {};

    Result(OkTag, T val) : m_value(std::move(val)) {}
    Result(FailTag, std::string err) : m_error(std::move(err)) {}

    std::optional<T> m_value;
    std::string m_error;
};

template <>
class Result<void> {
public:
    static Result ok() { return Result(); }
    static Result fail(std::string err) { return Result(std::move(err)); }

    bool isOk() const { return !m_hasError; }
    bool isFail() const { return m_hasError; }
    std::string & error() { return m_error; }
    const std::string & error() const { return m_error; }

private:
    Result() : m_hasError(false) {}
    Result(std::string err) : m_hasError(true), m_error(std::move(err)) {}

    bool m_hasError;
    std::string m_error;
};

#endif
