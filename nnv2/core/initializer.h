#pragma once

#include <vector>

namespace nnv2 {

class Initializer {
public:
    Initalizer() = default;

    virtual void initialize(std::vector<float> &data) = 0;
};

class LecunNormal: public Initializer {
public:
    LecunNormal(_fan_in): Initializer(), fan_in(_fan_in) {}

    void initialize(std::vector<float> &data) override;

private:
    int fan_in;
}

class XavierNormal: public Initializer {
public:
    XavierNormal(_fan_in, _fan_out): Initializer(), fan_in(_fan_in), fan_out(_fan_out) {}

    void initialize(std::vector<float> &data) override;

private:
    int fan_in;
    int fan_out;
}

class KaimingNormal: public Initializer {
public:
    KaimingNormal(_fan_in): Initializer(), fan_in(_fan_in) {}

    void initialize(std::vector<float> &data) override;

private:
    int fan_in;
}

class SimplyNormal: public Initializer {
public:
    SimplyNormal(): Initializer() {}

    void initialize(std::vector<float> &data) override;
}

class LecunUniform: public Initializer {
public:
    LecunUniform(_fan_in): Initializer(), fan_in(_fan_in) {}

    void initialize(std::vector<float> &data) override;

private:
    int fan_in;
}

class XavierUniform: public Initializer {
public:
    XavierUniform(_fan_in, _fan_out): Initializer(), fan_in(_fan_in), fan_out(_fan_out) {}

    void initialize(std::vector<float> &data) override;

private:
    int fan_in;
    int fan_out;
}

class KaimingUniform: public Initializer {
public:
    KaimingUniform(_fan_in): Initializer(), fan_in(_fan_in) {}

    void initialize(std::vector<float> &data) override;

private:
    int fan_in;
}

class SimplyUniform: public Initializer {
public:
    SimplyUniform(): Initializer() {}

    void initialize(std::vector<float> &data) override;
}

} // namespace nnv2