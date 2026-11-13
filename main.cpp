#include <cstdio>
#include <iostream>
#include <limits>
#include <string>
#include "travel.h"

using namespace std;

// ===== Helper input =====

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

string pilihKategori() {
    int n = cetakNomorKategori();
    if (n == 0) {
        cout << "Belum ada kategori.\n";
        return "";
    }
    int p = bacaAngka("Pilih nomor kategori: ", 1, n);
    return kategoriKe(p);
}

string pilihTempat(string kategori) {
    int n = cetakNomorTempat(kategori);
    if (n == 0) {
        cout << "Kategori ini belum punya tempat wisata.\n";
        return "";
    }
    int p = bacaAngka("Pilih nomor tempat: ", 1, n);
    return tempatKe(kategori, p);
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

// ===== Menu =====

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

void menuTambahKategori() {
    string nama = bacaTeks("Nama kategori: ");
    if (tambahKategori(nama)) {
        cout << "Kategori berhasil ditambahkan.\n";
    } else {
        cout << "Gagal: nama kosong atau kategori sudah ada.\n";
    }
}

void menuTambahTempat() {
    string kat = pilihKategori();
    if (kat.empty()) return;
    string nama = bacaTeks("Nama tempat wisata: ");
    if (tambahTempat(kat, nama)) {
        cout << "Tempat wisata berhasil ditambahkan.\n";
    } else {
        cout << "Gagal: nama kosong atau sudah ada di kategori ini.\n";
    }
}

void menuHapusTempat() {
    string kat = pilihKategori();
    if (kat.empty()) return;
    string tempat = pilihTempat(kat);
    if (tempat.empty()) return;

    string pesan;
    if (hapusTempat(kat, tempat, pesan)) {
        cout << pesan << "\n";
    } else {
        cout << "Gagal: " << pesan << "\n";
    }
}

void menuCariTiket() {
    string kode = bacaTeks("Masukkan kode booking: ");
    Pemesanan* p = cariPemesanan(kode);
    if (!p) {
        cout << "Tiket dengan kode tersebut tidak ditemukan.\n";
        return;
    }
    cetakDetailPemesanan(p);
}

void menuBooking() {
    string kat = pilihKategori();
    if (kat.empty()) return;
    string tempat = pilihTempat(kat);
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
    if (buatPemesanan(kat, tempat, pemesan, anggota, jenis, model, tanggal, kode, pesan)) {
        cout << "Booking berhasil. Kode booking Anda: " << kode << "\n";
    } else {
        cout << "Gagal: " << pesan << "\n";
    }
}

void menuTambahPenumpang() {
    string kode = bacaTeks("Masukkan kode booking: ");
    Pemesanan* p = cariPemesanan(kode);
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
    if (tambahPenumpang(kode, anggota, jenisBaru, modelBaru, pesan)) {
        cout << pesan << "\n";
    } else {
        cout << "Gagal: " << pesan << "\n";
    }
}

void menuKonfirmasiBayar() {
    string kode = bacaTeks("Masukkan kode booking: ");
    string pesan;
    if (konfirmasiBayar(kode, pesan)) {
        cout << pesan << "\n";
    } else {
        cout << "Gagal: " << pesan << "\n";
    }
}

void menuBatalkan() {
    string kode = bacaTeks("Masukkan kode booking: ");
    string pesan;
    if (batalkanPemesanan(kode, pesan)) {
        cout << pesan << "\n";
    } else {
        cout << "Gagal: " << pesan << "\n";
    }
}

void menuPemesananPerWisata() {
    string kat = pilihKategori();
    if (kat.empty()) return;
    string tempat = pilihTempat(kat);
    if (tempat.empty()) return;

    cout << "\nPemesanan untuk " << tempat << ":\n";
    tampilkanPemesananPerWisata(kat, tempat);
}

// ===== Data awal contoh =====

void dataAwal() {
    tambahKategori("Pegunungan");
    tambahTempat("Pegunungan", "Bromo");
    tambahTempat("Pegunungan", "Kawah Sikidang");
    tambahTempat("Pegunungan", "Baturaden");
    tambahTempat("Pegunungan", "Telomoyo");

    tambahKategori("Pantai");
    tambahTempat("Pantai", "Pantai Parangtritis");
    tambahTempat("Pantai", "Pantai Kuta");
}


// ===== Program utama =====
    
int main() {
    inisialisasiSistem();
    dataAwal();

    int pilihan = -1;
    bool jalan = true;

    while (jalan) {
        tampilkanMenu();
        pilihan = bacaAngka("Pilih menu: ", 0, 12);
        cout << "\n";

        if (pilihan == 1) {
            menuTambahKategori();
        }
        else if (pilihan == 2) {
            menuTambahTempat();
        }
        else if (pilihan == 3) {
            tampilkanKatalog();
        }
        
        else if (pilihan == 4) {
            menuHapusTempat();
        }
        else if (pilihan == 5) {
            menuCariTiket();
        }
        else if (pilihan == 6) {
            tampilkanWisataTerurut();
        }
        else if (pilihan == 7) {
            menuBooking();
        }
        else if (pilihan == 8) {
            menuTambahPenumpang();
        }
        else if (pilihan == 9) {
            menuKonfirmasiBayar();
        }
        else if (pilihan == 10) {
            menuBatalkan();
        }
        else if (pilihan == 11) {
            menuPemesananPerWisata();
        }
        else if (pilihan == 12) {
            tampilkanStatistik();
        }
        else if (pilihan == 0) {
            cout << "Terima kasih.\n";
            jalan = false;
        }
    }

    bersihkanSistem();

    return 0;
}
