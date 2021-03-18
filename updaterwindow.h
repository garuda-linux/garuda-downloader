#ifndef UPDATERWINDOW_H
#define UPDATERWINDOW_H

#include <QDialog>

namespace Ui {
class updaterwindow;
}

class UpdaterWindow : public QDialog {
  Q_OBJECT

public:
  explicit UpdaterWindow(QWidget *parent);
  ~UpdaterWindow();

private:
  Ui::updaterwindow *ui;

  void doUpdate(QWidget *parent);
};

#endif // UPDATERWINDOW_H
