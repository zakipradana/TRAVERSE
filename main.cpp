#include <cstdio>
#include <iostream>
#include <limits>
#include <string>
#include "travel.h"

using namespace std;



string bacaTeks(string prompt) {
    cout << prompt;
    string s;
    getline(cin, s);
    size_t awal = s.find_first_not_of(" \t");
    if (awal == string::npos) return "";
    size_t akhir = s.find_last_not_of(" \t");
    return s.substr(awal, akhir - awal + 1);
}

int bacaAngka(string prompt, int minimum, int maximum) {
    while (true) {
        string s = bacaTeks(prompt);
        try {
            size_t pos = 0;
            int n = stoi(s, &pos);
            if (pos == s.size() && n >= minimum && n <= maximum) return n;
        } catch (...) {
        }
        cout << "Masukan tidak valid, coba lagi.\n";
    }
}

Tanggal bacaTanggal() {
    while (true) {
        string s = bacaTeks("Tanggal keberangkatan (YYYY-MM-DD): ");
        int y = 0, m = 0, d = 0, n = 0;
        if (sscanf(s.c_str(), "%d-%d-%d%n", &y, &m, &d, &n) == 3 &&
            n == static_cast<int>(s.size()) && m >= 1 && m <= 12 && d >= 1 && d <= 31) {
            Tanggal t;
            t.tahun = y;
            t.bulan = m;
            t.hari = d;
            return t;
        }
        cout << "Format tanggal tidak valid.\n";
    }
}

string bacaJenisKelamin() {
    cout << "Jenis kelamin:\n  1. Laki-laki\n  2. Perempuan\n";
    int p = bacaAngka("Pilihan: ", 1, 2);
    if (p == 1) return "Laki-laki";
    return "Perempuan";
}

DataOrang bacaOrang() {
    DataOrang d;
    d.nama = bacaTeks("Nama lengkap: ");
    while (d.nama.empty()) {
        cout << "Nama tidak boleh kosong.\n";
        d.nama = bacaTeks("Nama lengkap: ");
    }
    d.umur = bacaAngka("Umur: ", 0, numeric_limits<int>::max());
    d.jk = bacaJenisKelamin();
    return d;
}

string pilihKategori(SistemTravel& sistem) {
    int n = sistem.cetakNomorKategori();
    if (n == 0) {
        cout << "Belum ada kategori.\n";
        return "";
    }
    int p = bacaAngka("Pilih nomor kategori: ", 1, n);
    return sistem.kategoriKe(p);
}

string pilihTempat(SistemTravel& sistem, string kategori) {
    int n = sistem.cetakNomorTempat(kategori);
    if (n == 0) {
        cout << "Kategori ini belum punya tempat wisata.\n";
        return "";
    }
    int p = bacaAngka("Pilih nomor tempat: ", 1, n);
    return sistem.tempatKe(kategori, p);
}

string pilihModel(int jenis) {
    cout << "Pilih model " << namaJenis(jenis) << ":\n";
    int n = jumlahModel(jenis);
    for (int i = 1; i <= n; i++) {
        cout << i << ". " << namaModel(jenis, i) << "\n";
    }
    int p = bacaAngka("Nomor model: ", 1, n);
    return namaModel(jenis, p);
}



void tampilkanMenu() {
    cout << "\n===== SISTEM PENGELOLAAN TRAVEL WISATA =====\n"
         << " 1. Tambah kategori wisata\n"
         << " 2. Tambah tempat wisata\n"
         << " 3. Tampilkan katalog\n"
         << " 4. Hapus tempat wisata\n"
         << " 5. Cari tiket (kode booking)\n"
         << " 6. Daftar wisata terurut\n"
         << " 7. Booking\n"
         << " 8. Tambah penumpang\n"
         << " 9. Konfirmasi pembayaran\n"
         << "10. Batalkan pemesanan\n"
         << "11. Lihat pemesanan per wisata\n"
         << "12. Statistik tempat paling ramai\n"
         << " 0. Keluar\n";
}

void menuTambahKategori(SistemTravel& sistem) {
    string nama = bacaTeks("Nama kategori: ");
    if (sistem.tambahKategori(nama)) {
        cout << "Kategori berhasil ditambahkan.\n";
    } else {
        cout << "Gagal: nama kosong atau kategori sudah ada.\n";
    }
}

void menuTambahTempat(SistemTravel& sistem) {
    string kat = pilihKategori(sistem);
    if (kat.empty()) return;
    string nama = bacaTeks("Nama tempat wisata: ");
    if (sistem.tambahTempat(kat, nama)) {
        cout << "Tempat wisata berhasil ditambahkan.\n";
    } else {
        cout << "Gagal: nama kosong atau sudah ada di kategori ini.\n";
    }
}

void menuHapusTempat(SistemTravel& sistem) {
    string kat = pilihKategori(sistem);
    if (kat.empty()) return;
    string tempat = pilihTempat(sistem, kat);
    if (tempat.empty()) return;

    string pesan;
    if (sistem.hapusTempat(kat, tempat, pesan)) {
        cout << pesan << "\n";
    } else {
        cout << "Gagal: " << pesan << "\n";
    }
}

void menuCariTiket(SistemTravel& sistem) {
    string kode = bacaTeks("Masukkan kode booking: ");
    Pemesanan* p = sistem.cariPemesanan(kode);
    if (!p) {
        cout << "Tiket dengan kode tersebut tidak ditemukan.\n";
        return;
    }
    cetakDetailPemesanan(p);
}

void menuBooking(SistemTravel& sistem) {
    string kat = pilihKategori(sistem);
    if (kat.empty()) return;
    string tempat = pilihTempat(sistem, kat);
    if (tempat.empty()) return;

    Tanggal tanggal = bacaTanggal();

    cout << "--- Data pemesan ---\n";
    DataOrang pemesan = bacaOrang();

    int jumlahAnggota = bacaAngka("Jumlah anggota rombongan selain pemesan: ", 0, 1000);
    Penumpang* anggota = nullptr;
    int kursi = 1;  // pemesan
    for (int i = 0; i < jumlahAnggota; i++) {
        cout << "--- Anggota ke-" << i + 1 << " ---\n";
        DataOrang a = bacaOrang();
        anggota = sisipAkhirPenumpang(anggota, a);
        kursi += kursiAnggota(a);
    }

    int jenis = 0;
    if (!jenisUntukKursi(kursi, jenis)) {
        hapusDaftarPenumpang(anggota);
        cout << "Gagal: total kursi " << kursi << " melebihi kapasitas bigbus (55).\n";
        return;
    }

    cout << "\nTotal tercatat : " << 1 + jumlahAnggota << " orang\n";
    cout << "Total kursi    : " << kursi << "\n";
    cout << "Sistem menyarankan: " << namaJenis(jenis)
         << " (kapasitas " << kapasitasJenis(jenis) << " kursi)\n";

    string model = pilihModel(jenis);

    string kode, pesan;
    if (sistem.buatPemesanan(kat, tempat, pemesan, anggota, jenis, model, tanggal, kode, pesan)) {
        cout << "Booking berhasil. Kode booking Anda: " << kode << "\n";
    } else {
        cout << "Gagal: " << pesan << "\n";
    }
}

void menuTambahPenumpang(SistemTravel& sistem) {
    string kode = bacaTeks("Masukkan kode booking: ");
    Pemesanan* p = sistem.cariPemesanan(kode);
    if (!p) {
        cout << "Kode booking tidak ditemukan.\n";
        return;
    }
    if (p->status == BATAL) {
        cout << "Pemesanan sudah dibatalkan.\n";
        return;
    }

    cout << "Kendaraan saat ini: " << namaJenis(p->jenis) << " - " << p->model
         << " (" << hitungKursi(p) << " kursi terpakai)\n";

    DataOrang anggota = bacaOrang();

    int kursiBaru = hitungKursi(p) + kursiAnggota(anggota);
    int jenisBaru = p->jenis;
    string modelBaru = p->model;

    if (kursiBaru > kapasitasJenis(p->jenis)) {
        if (!jenisUntukKursi(kursiBaru, jenisBaru)) {
            cout << "Penambahan ditolak: total kursi " << kursiBaru
                 << " melebihi kapasitas bigbus (55).\n";
            return;
        }
        cout << "Total kursi menjadi " << kursiBaru << ", kendaraan perlu diganti ke "
             << namaJenis(jenisBaru) << ".\n";
        if (p->status == LUNAS) {
            cout << "Catatan: pemesanan sudah lunas, status akan kembali menjadi belum lunas.\n";
        }
        modelBaru = pilihModel(jenisBaru);
    }

    string pesan;
    if (sistem.tambahPenumpang(kode, anggota, jenisBaru, modelBaru, pesan)) {
        cout << pesan << "\n";
    } else {
        cout << "Gagal: " << pesan << "\n";
    }
}

void menuKonfirmasiBayar(SistemTravel& sistem) {
    string kode = bacaTeks("Masukkan kode booking: ");
    string pesan;
    if (sistem.konfirmasiBayar(kode, pesan)) {
        cout << pesan << "\n";
    } else {
        cout << "Gagal: " << pesan << "\n";
    }
}

void menuBatalkan(SistemTravel& sistem) {
    string kode = bacaTeks("Masukkan kode booking: ");
    string pesan;
    if (sistem.batalkanPemesanan(kode, pesan)) {
        cout << pesan << "\n";
    } else {
        cout << "Gagal: " << pesan << "\n";
    }
}

void menuPemesananPerWisata(SistemTravel& sistem) {
    string kat = pilihKategori(sistem);
    if (kat.empty()) return;
    string tempat = pilihTempat(sistem, kat);
    if (tempat.empty()) return;

    cout << "\nPemesanan untuk " << tempat << ":\n";
    sistem.tampilkanPemesananPerWisata(kat, tempat);
}



void dataAwal(SistemTravel& s) {
    s.tambahKategori("Pegunungan");
    s.tambahTempat("Pegunungan", "Bromo");
    s.tambahTempat("Pegunungan", "Kawah Sikidang");
    s.tambahTempat("Pegunungan", "Baturaden");
    s.tambahTempat("Pegunungan", "Telomoyo");

    s.tambahKategori("Pantai");
    s.tambahTempat("Pantai", "Pantai Parangtritis");
    s.tambahTempat("Pantai", "Pantai Kuta");
}



int main() {
    SistemTravel sistem;
    dataAwal(sistem);

    int pilihan;
    do {
        tampilkanMenu();
        pilihan = bacaAngka("Pilih menu: ", 0, 12);
        cout << "\n";

        switch (pilihan) {
            case 1:  menuTambahKategori(sistem); break;
            case 2:  menuTambahTempat(sistem); break;
            case 3:  sistem.tampilkanKatalog(); break;
            case 4:  menuHapusTempat(sistem); break;
            case 5:  menuCariTiket(sistem); break;
            case 6:  sistem.tampilkanWisataTerurut(); break;
            case 7:  menuBooking(sistem); break;
            case 8:  menuTambahPenumpang(sistem); break;
            case 9:  menuKonfirmasiBayar(sistem); break;
            case 10: menuBatalkan(sistem); break;
            case 11: menuPemesananPerWisata(sistem); break;
            case 12: sistem.tampilkanStatistik(); break;
            case 0:  cout << "Terima kasih.\n"; break;
        }
    } while (pilihan != 0);

    return 0;
}