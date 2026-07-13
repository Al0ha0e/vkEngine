#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include <common.hpp>
#include <cstdint>

namespace vke_editor
{
    enum class EditorState : int32_t
    {
        PartialInited = 0,
        Edit = 1,
        Run = 2,
    };

    class EditorStateManager
    {
    private:
        static EditorStateManager *instance;
        EditorStateManager()
            : state(EditorState::PartialInited)
        {
        }
        ~EditorStateManager() {}
        EditorStateManager(const EditorStateManager &);
        EditorStateManager &operator=(const EditorStateManager &);

    public:
        EditorState state;

        static EditorStateManager *GetInstance()
        {
            return instance;
        }

        static EditorStateManager *Init(EditorState initialState)
        {
            if (instance == nullptr)
                instance = new EditorStateManager();
            SetState(initialState);
            return instance;
        }

        static void Dispose()
        {
            if (instance == nullptr)
                return;
            delete instance;
            instance = nullptr;
        }

        static void SetState(EditorState nextState)
        {
            instance->state = nextState;
        }

        static EditorState GetState()
        {
            return instance->state;
        }
    };
}

#endif
