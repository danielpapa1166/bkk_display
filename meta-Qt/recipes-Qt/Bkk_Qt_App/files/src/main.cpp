#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QLabel label("Szia Ági!");
    label.setMinimumSize(400, 200);
    label.setAlignment(Qt::AlignCenter);
    label.show();

    return app.exec();
}
