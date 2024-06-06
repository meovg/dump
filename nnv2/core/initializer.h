#pragma once

#include <vector>

namespace nnv2 {

class Initializer {
public:
    Initializer() {};

    virtual void initialize(std::vector<float> &data) const = 0;
};

class LecunNormal: public Initializer {
public:
    LecunNormal(int _fan_in): Initializer(), fan_in(_fan_in) {}

    void initialize(std::vector<float> &data) const override;

private:
    int fan_in;
};

class XavierNormal: public Initializer {
public:
    XavierNormal(int _fan_in, int _fan_out): Initializer(), fan_in(_fan_in), fan_out(_fan_out) {}

    void initialize(std::vector<float> &data) const override;

private:
    int fan_in;
    int fan_out;
};

class KaimingNormal: public Initializer {
public:
    KaimingNormal(int _fan_in): Initializer(), fan_in(_fan_in) {}

    void initialize(std::vector<float> &data) const override;

private:
    int fan_in;
};

class SimplyNormal: public Initializer {
public:
    SimplyNormal(): Initializer() {}

    void initialize(std::vector<float> &data) const override;
};

class LecunUniform: public Initializer {
public:
    LecunUniform(int _fan_in): Initializer(), fan_in(_fan_in) {}

    void initialize(std::vector<float> &data) const override;

private:
    int fan_in;
};

class XavierUniform: public Initializer {
public:
    XavierUniform(int _fan_in, int _fan_out): Initializer(), fan_in(_fan_in), fan_out(_fan_out) {}

    void initialize(std::vector<float> &data) const override;

private:
    int fan_in;
    int fan_out;
};

class KaimingUniform: public Initializer {
public:
    KaimingUniform(int _fan_in): Initializer(), fan_in(_fan_in) {}

    void initialize(std::vector<float> &data) const override;

private:
    int fan_in;
};

class SimplyUniform: public Initializer {
public:
    SimplyUniform(): Initializer() {}

    void initialize(std::vector<float> &data) const override;
};

} // namespace nnv2