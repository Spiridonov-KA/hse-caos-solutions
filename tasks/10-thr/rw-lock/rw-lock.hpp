#pragma once

class RWLock {
  public:
    RWLock() {
    }

    RWLock(const RWLock&) = delete;
    RWLock& operator=(const RWLock&) = delete;

    RWLock(RWLock&&) = delete;
    RWLock& operator=(RWLock&&) = delete;

    // Lock for read, shared access
    void ReadLock() {
        // TODO: your code here.
    }

    // Lock for write, exclusive access
    void WriteLock() {
        // TODO: your code here.
    }

    // Try lock for read, shared access
    bool ReadTryLock() {
        return false;  // TODO: remove before flight.
    }

    // Try lock for write, exclusive access
    bool WriteTryLock() {
        return false;  // TODO: remove before flight.
    }

    void WriteUnlock() {
        // TODO: your code here.
    }

    void ReadUnlock() {
        // TODO: your code here.
    }

};
