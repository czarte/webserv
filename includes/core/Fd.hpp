#ifndef FD_HPP
#define FD_HPP

class Fd
{
    public:
        Fd();
        explicit Fd(int fd);
        ~Fd();

        int get() const;
        int release();

    private:
        Fd(const Fd &);
        Fd &operator=(const Fd &);

        int _fd;
};

#endif 
