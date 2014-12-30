#ifndef GRAPEVINE_INCLUDE_GRAPEVINE_H_
#define GRAPEVINE_INCLUDE_GRAPEVINE_H_

enum GV_FLAG : unsigned int
{
    GV_FLAG_BLOCK           = 1 << 0,
    GV_FLAG_NO_BLOCK        = 1 << 1
};

class GVPublisher
{
    public:
        int publish_message(GV_FLAG flags, char const *msg);
        int register_service(char const *srv);
    private:
};

class GVSubscriber
{
    public:
        int subscribe(char const &srv);
        int get_last_message(GV_FLAG flags, char &msg);
        int get_next_message(GV_FLAG flags, char &msg);
        int get_message_at(int idx, char &msg);
    private:
};

#endif // GRAPEVINE_INCLUDE_GRAPEVINE_H_

