#include <lshkit.h>

using namespace std;
using namespace lshkit;

int main (int argc, char *argv[])
{
    FloatMatrix data(argv[1]);
    SpectralHash hash;
    {
        ifstream is(argv[2], ios::binary);
        hash.serialize(is, 0);
    }

    for (int i = 0; i < data.getSize(); ++i) {
        cout << hash(data[i]) << endl;
    }

    return 0;
}
