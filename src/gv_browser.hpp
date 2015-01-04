#ifndef GRAPEVINE_SRC_GV_BROWSER_HPP_
#define GRAPEVINE_SRC_GV_BROWSER_HPP_

// FIXME cevans87: What should the callback give?
typedef int (*GV_Callback)(void);

class GVBrowser
{
    public:
        int enable();
        int disable();
        int register_callback(GV_Callback callback);
    private:
        GV_Callback _callback;
};

#endif // GRAPEVINE_SRC_GV_BROWSER_HPP_

