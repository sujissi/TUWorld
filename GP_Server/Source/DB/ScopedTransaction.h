#pragma once

class ScopedTransaction {
public:
    ScopedTransaction(mysqlx::Session& sess) : _sess(sess), _committed(false) {
        _sess.startTransaction();
    }
    ~ScopedTransaction() {
        if (!_committed) _sess.rollback();
    }
    ScopedTransaction(const ScopedTransaction&) = delete;
    ScopedTransaction& operator=(const ScopedTransaction&) = delete;

    void Commit() {
        _sess.commit();
        _committed = true;
    }

private:
    mysqlx::Session& _sess;
    bool _committed = false;
};
